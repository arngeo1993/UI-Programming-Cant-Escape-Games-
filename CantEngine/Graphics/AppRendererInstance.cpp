#include "stdafx.h"
#include "AppRendererInstance.h"
#include "GraphicsSettings.h"
#include "Graphics/Deferred/DeferredRenderingInstance.h"
#include "Graphics/DebugRendering/DebugRenderingInstance.h"
#include "Graphics/PostEffects/MSAAResolvePassInstance.h"
#include "Graphics/Particles/ParticleRenderingInstance.h"
#include "Graphics/UI_Rendering/UIObjectRenderingInstance.h"
#include "Graphics/TextRendering/TextRenderingInstance.h"
#include "Managers/CameraManager.h"
#include "Graphics/Camera.h"
#include "Graphics/AppRenderer.h"
#include "Graphics/Models/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Light.h"

AppRendererInstance::AppRendererInstance(AppRenderer* appRenderer,
	DXRenderer* dxrenderer, const CameraInfo& cameraInfo)
	:m_appRenderer(appRenderer),
	m_dxrenderer(dxrenderer),
	m_context(cameraInfo)
{
	m_context.m_appRendererInstance = this;

	m_deferredRenderingInstance = new DeferredRenderingInstance(appRenderer->m_deferrredRendering);
	m_debugRenderingInstance = new DebugRenderingInstance(appRenderer->m_debugRendering);
	m_msaaResolvePassInstance = new MSAAResolvePassInstance(appRenderer->m_msaa_resolve_pass);
	m_particleRenderingInstance = new ParticleRenderingInstance(appRenderer->m_particleRendering);
	m_uiObjectRenderingInstance = new UIObjectRenderingInstance(appRenderer->m_uiObjectRendering);
	m_textRenderingInstance = new TextRenderingInstance(appRenderer->m_textRendering);
}


AppRendererInstance::~AppRendererInstance()
{
}

void AppRendererInstance::Release()
{
	for (Buffer* buffer : m_objectUniformBufferList)
	{
		SafeReleaseDelete(buffer);
	}
	for (Buffer* buffer : m_boneMeshObjectUniformBufferList)
	{
		SafeReleaseDelete(buffer);
	}

	SafeReleaseDelete(m_debugRenderingInstance);
	SafeReleaseDelete(m_deferredRenderingInstance);
	SafeReleaseDelete(m_msaaResolvePassInstance);
	SafeReleaseDelete(m_particleRenderingInstance);
	SafeReleaseDelete(m_uiObjectRenderingInstance);
	SafeReleaseDelete(m_textRenderingInstance);

	SafeReleaseDelete(m_camera_uniform_buffer);
	SafeReleaseDelete(m_resolveUniformBuffer);

	SafeReleaseDelete(m_finalOutputRT);
	SafeReleaseDelete(m_msaaMainRT);
	SafeReleaseDelete(m_depthRT);
	SafeReleaseDelete(m_skyboxUniformBuffer);
}

void AppRendererInstance::Initialize()
{
	BufferLoadDesc camera_uniform_buffer_desc = {};
	camera_uniform_buffer_desc.m_desc.m_bindFlags = Bind_Flags::BIND_CONSTANT_BUFFER;
	camera_uniform_buffer_desc.m_desc.m_cpuAccessType = CPU_Access_Type::ACCESS_WRITE;
	camera_uniform_buffer_desc.m_desc.m_usageType = Usage_Type::USAGE_DYNAMIC;
	camera_uniform_buffer_desc.m_desc.m_debugName = "Camera Uniform Buffer";
	camera_uniform_buffer_desc.m_rawData = nullptr;
	camera_uniform_buffer_desc.m_size = sizeof(CameraUniformData);
	m_camera_uniform_buffer = DXResourceLoader::Create_Buffer(m_dxrenderer, camera_uniform_buffer_desc);


	BufferLoadDesc skybox_uniform_buffer_desc = {};
	skybox_uniform_buffer_desc.m_desc.m_bindFlags = Bind_Flags::BIND_CONSTANT_BUFFER;
	skybox_uniform_buffer_desc.m_desc.m_cpuAccessType = CPU_Access_Type::ACCESS_WRITE;
	skybox_uniform_buffer_desc.m_desc.m_usageType = Usage_Type::USAGE_DYNAMIC;
	skybox_uniform_buffer_desc.m_desc.m_debugName = "Skybox camera uniform data";
	skybox_uniform_buffer_desc.m_size = sizeof(SkyboxUniformData);
	skybox_uniform_buffer_desc.m_rawData = nullptr;
	m_skyboxUniformBuffer = DXResourceLoader::Create_Buffer(m_dxrenderer, skybox_uniform_buffer_desc);


	const Vector4& viewportRenderInformation = m_context.m_cameraInfo.m_camera.GetViewportRenderInformation();
	const Vector2 ndcLocation(viewportRenderInformation.x, viewportRenderInformation.y);

	ResolveRendererInstancesUniformData resolveRendererInstancesUniformData = {};
	resolveRendererInstancesUniformData.Translation = Vector4(ndcLocation.x * 2.f, ndcLocation.y * 2.f, 0.f, 0.f);
	Vector2 ndcScaleLoc = Vector2((viewportRenderInformation.z - 0.5f) * 2.f, (viewportRenderInformation.w - 0.5f) * -2.f);
	resolveRendererInstancesUniformData.Scale = Vector4(ndcScaleLoc.x, ndcScaleLoc.y, 0.f, 0.f);


	BufferLoadDesc resolveAppRendererUniformDesc = {};
	resolveAppRendererUniformDesc.m_desc.m_bindFlags = Bind_Flags::BIND_CONSTANT_BUFFER;
	resolveAppRendererUniformDesc.m_desc.m_debugName = "Resolve Renderer Instances Uniform Buffer";
	resolveAppRendererUniformDesc.m_desc.m_cpuAccessType = CPU_Access_Type::ACCESS_NONE;
	resolveAppRendererUniformDesc.m_desc.m_usageType = Usage_Type::USAGE_DEFAULT;
	resolveAppRendererUniformDesc.m_rawData = &resolveRendererInstancesUniformData;
	resolveAppRendererUniformDesc.m_size = sizeof(ResolveRendererInstancesUniformData);

	m_resolveUniformBuffer = DXResourceLoader::Create_Buffer(
		m_dxrenderer, resolveAppRendererUniformDesc);

	m_deferredRenderingInstance->Initialize(m_context);
	m_msaaResolvePassInstance->Initialize(m_context);
	m_debugRenderingInstance->Initialize();
	m_particleRenderingInstance->Initialize(m_context);
	m_uiObjectRenderingInstance->Initialize(m_context);
	m_textRenderingInstance->Initialize(m_context);
}

void AppRendererInstance::LoadContent()
{
	RenderTarget* swap_chain_rt = m_dxrenderer->GetSwapChain()->m_p_swap_chain_render_target;

	const Vector4& viewPortRenderInformation = m_context.m_cameraInfo.m_camera.GetViewportRenderInformation();
	int32_t finalRTWidth = (int32_t) (viewPortRenderInformation.z * (float)swap_chain_rt->get_desc().m_texture_desc.m_width);
	int32_t finalRTHeight = (int32_t)(viewPortRenderInformation.w * (float)swap_chain_rt->get_desc().m_texture_desc.m_height);

	RenderTargetDesc depth_rt_desc = {};
	depth_rt_desc.m_texture_desc.m_bindFlags = Bind_Flags::BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depth_rt_desc.m_texture_desc.m_cpuAccessType = CPU_Access_Type::ACCESS_NONE;
	depth_rt_desc.m_texture_desc.m_width = finalRTWidth;
	depth_rt_desc.m_texture_desc.m_height = finalRTHeight;
	depth_rt_desc.m_texture_desc.m_clearVal = ClearValue{ 1.0f, 0.0 };
	depth_rt_desc.m_texture_desc.m_mipLevels = 1;
	depth_rt_desc.m_texture_desc.m_imageFormat = DXGI_FORMAT_D32_FLOAT;
	depth_rt_desc.m_texture_desc.m_usageType = Usage_Type::USAGE_DEFAULT;
	depth_rt_desc.m_texture_desc.m_depth = 1;
	depth_rt_desc.m_texture_desc.m_sampleCount = (SampleCount)GraphicsSettings::MSAA_SAMPLE_COUNT;
	m_depthRT = DXResourceLoader::Create_RenderTarget(m_dxrenderer, depth_rt_desc);


	RenderTargetDesc msaa_rt_desc = {};
	msaa_rt_desc.m_texture_desc.m_bindFlags = Bind_Flags::BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
	msaa_rt_desc.m_texture_desc.m_cpuAccessType = CPU_Access_Type::ACCESS_NONE;
	msaa_rt_desc.m_texture_desc.m_width = finalRTWidth;
	msaa_rt_desc.m_texture_desc.m_height = finalRTHeight;
	msaa_rt_desc.m_texture_desc.m_clearVal = ClearValue{ 0.f, 0.0, 0.f, 0.f };
	msaa_rt_desc.m_texture_desc.m_mipLevels = 1;
	msaa_rt_desc.m_texture_desc.m_sampleCount = (SampleCount)GraphicsSettings::MSAA_SAMPLE_COUNT;
	msaa_rt_desc.m_texture_desc.m_imageFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	msaa_rt_desc.m_texture_desc.m_usageType = Usage_Type::USAGE_DEFAULT;
	msaa_rt_desc.m_texture_desc.m_depth = 1;
	m_msaaMainRT = DXResourceLoader::Create_RenderTarget(m_dxrenderer, msaa_rt_desc);

	msaa_rt_desc.m_texture_desc.m_sampleCount = SAMPLE_COUNT_1;
	m_finalOutputRT = DXResourceLoader::Create_RenderTarget(m_dxrenderer, msaa_rt_desc);

	if (GraphicsSettings::MSAA_SAMPLE_COUNT > 1)
	{
		m_curMainRT = m_msaaMainRT;
	}
	else
	{
		m_curMainRT = m_finalOutputRT;
	}

	m_deferredRenderingInstance->LoadContent(m_context);
	m_msaaResolvePassInstance->LoadContent(m_context);
	m_particleRenderingInstance->LoadContent(m_context);
	m_uiObjectRenderingInstance->LoadContent(m_context);
	m_textRenderingInstance->LoadContent(m_context);
	m_debugRenderingInstance->LoadContent();
}


void AppRendererInstance::Update(float dt)
{
	Camera& mainCamera =  m_context.m_cameraInfo.m_camera;

	const TextureDesc& finalTextureDesc = m_finalOutputRT->get_desc().m_texture_desc;
	Vector3 camera_pos = mainCamera.GetCameraPosition();
	m_camera_uniform_data.CameraPosition = MathUtil::v3_to_v4(camera_pos, 1.f);
	m_camera_uniform_data.ViewMat = mainCamera.GetViewMatrix();
	m_camera_uniform_data.ProjectionMat = mainCamera.GetProjectionMatrix();
	m_camera_uniform_data.InvProjectionMat = mainCamera.GetInvProjectionMatrix();
	m_camera_uniform_data.InvViewMat = mainCamera.GetInvViewMatrix();
	m_camera_uniform_data.ViewProjectionMat = mainCamera.GetViewProjectionMatrix();
	m_camera_uniform_data.InvViewProjectionMat = mainCamera.GetInvViewProjectionMatrix();
	m_camera_uniform_data.CameraViewportSize = Vector2((float)finalTextureDesc.m_width,
		(float)finalTextureDesc.m_height);

	Matrix no_position_view_mat = m_camera_uniform_data.ViewMat;
	no_position_view_mat._41 = 0.f;
	no_position_view_mat._42 = 0.f;
	no_position_view_mat._43 = 0.f;

	m_skyboxUniformData.ModelViewProjectionMat =
		no_position_view_mat * m_camera_uniform_data.ProjectionMat;

	m_debugRenderingInstance->Update(m_context, dt);
}


void AppRendererInstance::Render()
{
	Camera& thisCamera = m_context.m_cameraInfo.m_camera;

	BufferUpdateDesc update_camera_desc = {};
	update_camera_desc.m_buffer = m_camera_uniform_buffer;
	update_camera_desc.m_pSource = &m_camera_uniform_data;
	update_camera_desc.m_size = sizeof(CameraUniformData);
	m_dxrenderer->cmd_update_buffer(update_camera_desc);

	uint32_t cameraRenderObjectType = thisCamera.GetCameraRenderObjectType();
	if ((cameraRenderObjectType & CAMERA_RENDER_MESHES) != 0)
	{
		LoadActionsDesc load_actions_desc = {};
		load_actions_desc.m_clear_color_values[0] = m_curMainRT->get_clear_value();
		load_actions_desc.m_load_actions_color[0] = LoadActionType::CLEAR;
		load_actions_desc.m_clear_depth_stencil = m_depthRT->get_clear_value();
		load_actions_desc.m_load_action_depth = LoadActionType::CLEAR;

		m_dxrenderer->cmd_bind_render_targets(&m_curMainRT, 1, m_depthRT, load_actions_desc);
		m_dxrenderer->cmd_set_viewport(0, 0, m_curMainRT->get_desc().m_texture_desc.m_width,
			m_curMainRT->get_desc().m_texture_desc.m_height);

		m_deferredRenderingInstance->Render(m_context);
		m_particleRenderingInstance->Render(m_context);

		if (GraphicsSettings::MSAA_SAMPLE_COUNT > 1)
		{
			m_msaaResolvePassInstance->Render(m_context);
		}
		else
		{
			LoadActionsDesc next_load_actions_desc = {};
			next_load_actions_desc.m_clear_color_values[0] = m_finalOutputRT->get_clear_value();
			next_load_actions_desc.m_load_actions_color[0] = LoadActionType::DONT_CLEAR;
			next_load_actions_desc.m_clear_depth_stencil = m_depthRT->get_clear_value();
			next_load_actions_desc.m_load_action_depth = LoadActionType::DONT_CLEAR;

			m_dxrenderer->cmd_bind_render_targets(&m_finalOutputRT, 1, nullptr, next_load_actions_desc);
			m_dxrenderer->cmd_set_viewport(0, 0, m_finalOutputRT->get_desc().m_texture_desc.m_width,
				m_finalOutputRT->get_desc().m_texture_desc.m_height);
		}
		RenderSkybox();
	}
	else
	{
		LoadActionsDesc next_load_actions_desc = {};
		next_load_actions_desc.m_clear_color_values[0] = m_finalOutputRT->get_clear_value();
		next_load_actions_desc.m_load_actions_color[0] = LoadActionType::CLEAR;
		next_load_actions_desc.m_clear_depth_stencil = m_depthRT->get_clear_value();
		next_load_actions_desc.m_load_action_depth = LoadActionType::DONT_CLEAR;

		m_dxrenderer->cmd_bind_render_targets(&m_finalOutputRT, 1, nullptr, next_load_actions_desc);
		m_dxrenderer->cmd_set_viewport(0, 0, m_finalOutputRT->get_desc().m_texture_desc.m_width,
			m_finalOutputRT->get_desc().m_texture_desc.m_height);
	}

	if ((cameraRenderObjectType & CAMERA_RENDER_UI) != 0)
	{
		m_uiObjectRenderingInstance->Render(m_context);
	}

	m_debugRenderingInstance->Render(m_context);
	m_textRenderingInstance->Render(m_context);
	m_lastMaterialIndex = 0;
}


void AppRendererInstance::RenderBasicInstances(Pipeline* pipeline)
{
	if ( m_appRenderer->m_basicInstances.empty())
	{
		return;
	}

	m_dxrenderer->cmd_bind_pipeline(pipeline);

	uint32_t material_index = 0;
	uint32_t basicInstanceIndex = 0;
	for (uint64_t i = 0; i < m_appRenderer->m_basicInstances.size(); ++i)
	{
		const InstanceRenderData& inst_data = m_appRenderer->m_basicInstances[i];
		Model* p_ref_model = inst_data.p_ref_model;
		Material* p_ref_material = inst_data.p_ref_material;

		assert(p_ref_material && p_ref_model);

		if (basicInstanceIndex >= m_objectUniformBufferList.size())
		{
			m_appRenderer->AddObjectUniformBuffer(m_objectUniformBufferList, m_objectUniformDataList);
		}

		Buffer* obj_uniform_buffer = m_objectUniformBufferList[basicInstanceIndex];
		m_objectUniformDataList[basicInstanceIndex] = {};
		m_objectUniformDataList[basicInstanceIndex].ModelMat = inst_data.model_mat;
		m_objectUniformDataList[basicInstanceIndex].InvModelMat = DirectX::XMMatrixInverse(nullptr, inst_data.model_mat);
		m_objectUniformDataList[basicInstanceIndex].ModelViewProjectionMat = inst_data.model_mat * m_camera_uniform_data.ViewProjectionMat;
		m_objectUniformDataList[basicInstanceIndex].NormalMat = inst_data.normal_mat;

		BufferUpdateDesc update_object_uniform_desc = {};
		update_object_uniform_desc.m_buffer = obj_uniform_buffer;
		update_object_uniform_desc.m_pSource = &m_objectUniformDataList[basicInstanceIndex];
		update_object_uniform_desc.m_size = sizeof(ObjectUniformData);
		m_dxrenderer->cmd_update_buffer(update_object_uniform_desc);


		m_dxrenderer->cmd_bind_vertex_buffer(inst_data.p_ref_model->GetVertexBuffer());

		Buffer* index_buffer = p_ref_model->GetIndexBuffer();
		if (index_buffer)
		{
			m_dxrenderer->cmd_bind_index_buffer(index_buffer);
		}

		DescriptorData objectParams[20] = {};
		objectParams[0].m_binding_location = 0;
		objectParams[0].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
		objectParams[0].m_shader_stages = Shader_Stages::VERTEX_STAGE | Shader_Stages::PIXEL_STAGE;
		objectParams[0].m_buffers = &m_camera_uniform_buffer;

		objectParams[1].m_binding_location = 1;
		objectParams[1].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
		objectParams[1].m_shader_stages = Shader_Stages::VERTEX_STAGE | Shader_Stages::PIXEL_STAGE;
		objectParams[1].m_buffers = &obj_uniform_buffer;

		objectParams[2].m_binding_location = 0;
		objectParams[2].m_descriptor_type = DescriptorType::DESCRIPTOR_SAMPLER;
		objectParams[2].m_shader_stages = Shader_Stages::PIXEL_STAGE;
		objectParams[2].m_samplers = &m_appRenderer->m_texture_sampler;

		m_dxrenderer->cmd_bind_descriptor(pipeline, 3, objectParams);


		const Model::MeshesList& meshes_list = p_ref_model->GetMeshesList();
		uint32_t mesh_instance_count = std::max(1u, static_cast<unsigned int>(meshes_list.size()));
		++basicInstanceIndex;
		for (uint32_t mesh_index = 0; mesh_index < mesh_instance_count; ++mesh_index)
		{
			Material* cur_material_instance = m_appRenderer->m_materialReferenceList[material_index];
			Buffer* material_uniform_buffer = m_appRenderer->m_material_uniform_buffer_list[material_index];
			uint32_t mat_id = (uint32_t)m_appRenderer->m_material_uniform_data_list[material_index].MaterialMiscData.w;

			DescriptorData params[20] = {};
			params[0].m_binding_location = 2;
			params[0].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
			params[0].m_shader_stages = Shader_Stages::VERTEX_STAGE | Shader_Stages::PIXEL_STAGE;
			params[0].m_buffers = &m_appRenderer->m_material_uniform_buffer_list[material_index];

			uint32_t total_params_count = 1;

			Texture* diffuse_texture = cur_material_instance->GetDiffuseTexture();
			Texture* normal_texture = cur_material_instance->GetNormalTexture();
			Texture* height_texture = cur_material_instance->GetHeightTexture();
			Texture* specularTexture = cur_material_instance->GetSpecularTexture();
			Texture* metallicTexture = cur_material_instance->GetMetallicTexture();
			Texture* roughnessTexture = cur_material_instance->GetRoughnessTexture();

			++material_index;
			if ((mat_id & (uint32_t)MAT_ID_DIFFUSE_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 0;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &diffuse_texture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_NORMAL_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 1;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &normal_texture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_PARALLAX_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 2;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &height_texture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_SPECULAR_TEXUTERE) != 0)
			{
				params[total_params_count].m_binding_location = 3;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &specularTexture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_METALLIC_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 4;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &metallicTexture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_ROUGHNESS_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 5;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &roughnessTexture;
				++total_params_count;
			}

			m_dxrenderer->cmd_bind_descriptor(pipeline, total_params_count, params);
			if (meshes_list.size() <= 0)
			{
				m_dxrenderer->cmd_draw_index(p_ref_model->GetIndexTotalCount(), 0, 0);
			}
			else
			{
				const Mesh& mesh_instance = meshes_list[mesh_index];
				m_dxrenderer->cmd_draw_index(mesh_instance.get_index_count(), mesh_instance.get_start_index(), mesh_instance.get_start_vertex());
			}
		}
	}

	m_lastMaterialIndex = material_index;
}




void AppRendererInstance::RenderBoneMeshInstances(Pipeline* pipeline)
{
	if (m_appRenderer->m_boneMeshInstancesList.empty())
	{
		return;
	}

	m_dxrenderer->cmd_bind_pipeline(pipeline);

	uint32_t material_index = 0;
	uint32_t basicInstanceIndex = 0;

	//HERE I AM ASSUMING THAT BASIC MESH IS BEIGN DRAWN BEFORE THE ANIMATED MODEL :)
	material_index = m_lastMaterialIndex;

	for (uint32_t i = 0; i < m_appRenderer->m_boneMeshInstancesList.size(); ++i)
	{
		const InstanceRenderData& inst_data = m_appRenderer->m_boneMeshInstancesList[i].m_instanceRenderData;
		Model* p_ref_model = inst_data.p_ref_model;
		Material* p_ref_material = inst_data.p_ref_material;

		assert(p_ref_model && p_ref_material);

		if (i >= m_boneMeshObjectUniformBufferList.size())
		{
			m_appRenderer->AddObjectUniformBuffer(m_boneMeshObjectUniformBufferList, m_boneMeshObjectUniformDataList);
		}



		Buffer* obj_uniform_buffer = m_boneMeshObjectUniformBufferList[i];

		m_boneMeshObjectUniformDataList[i] = {};
		m_boneMeshObjectUniformDataList[i].ModelMat = inst_data.model_mat;
		m_boneMeshObjectUniformDataList[i].InvModelMat = DirectX::XMMatrixInverse(nullptr, m_boneMeshObjectUniformDataList[i].ModelMat);
		m_boneMeshObjectUniformDataList[i].ModelViewProjectionMat = m_boneMeshObjectUniformDataList[i].ModelMat * m_camera_uniform_data.ViewProjectionMat;
		m_boneMeshObjectUniformDataList[i].NormalMat = inst_data.normal_mat;

		BufferUpdateDesc update_object_uniform_desc = {};
		update_object_uniform_desc.m_buffer = obj_uniform_buffer;
		update_object_uniform_desc.m_pSource = &m_boneMeshObjectUniformDataList[i];
		update_object_uniform_desc.m_size = sizeof(ObjectUniformData);
		m_dxrenderer->cmd_update_buffer(update_object_uniform_desc);


		Buffer* bone_uniform_buffer = m_appRenderer->m_boneTransformsUniformBufferList[i];


		m_dxrenderer->cmd_bind_vertex_buffer(inst_data.p_ref_model->GetVertexBuffer());

		Buffer* index_buffer = p_ref_model->GetIndexBuffer();
		if (index_buffer)
		{
			m_dxrenderer->cmd_bind_index_buffer(index_buffer);
		}

		DescriptorData objectParams[20] = {};
		objectParams[0].m_binding_location = 0;
		objectParams[0].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
		objectParams[0].m_shader_stages = Shader_Stages::VERTEX_STAGE | Shader_Stages::PIXEL_STAGE;
		objectParams[0].m_buffers = &m_camera_uniform_buffer;

		objectParams[1].m_binding_location = 1;
		objectParams[1].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
		objectParams[1].m_shader_stages = Shader_Stages::VERTEX_STAGE | Shader_Stages::PIXEL_STAGE;
		objectParams[1].m_buffers = &obj_uniform_buffer;

		objectParams[2].m_binding_location = 0;
		objectParams[2].m_descriptor_type = DescriptorType::DESCRIPTOR_SAMPLER;
		objectParams[2].m_shader_stages = Shader_Stages::PIXEL_STAGE;
		objectParams[2].m_samplers = &m_appRenderer->m_texture_sampler;

		objectParams[3].m_binding_location = 3;
		objectParams[3].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
		objectParams[3].m_shader_stages = Shader_Stages::VERTEX_STAGE;
		objectParams[3].m_buffers = &bone_uniform_buffer;

		m_dxrenderer->cmd_bind_descriptor(pipeline, 4, objectParams);

		const Model::MeshesList& meshes_list = p_ref_model->GetMeshesList();
		uint32_t mesh_instance_count = std::max(1u, (uint32_t)meshes_list.size());

		for (uint32_t mesh_index = 0; mesh_index < mesh_instance_count; ++mesh_index)
		{
			Material* cur_material_instance = m_appRenderer->m_materialReferenceList[material_index];
			Buffer* material_uniform_buffer = m_appRenderer->m_material_uniform_buffer_list[material_index];

			uint32_t mat_id = (uint32_t)m_appRenderer->m_material_uniform_data_list[material_index].MaterialMiscData.w;

			DescriptorData params[20] = {};
			params[0].m_binding_location = 2;
			params[0].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
			params[0].m_shader_stages = Shader_Stages::VERTEX_STAGE | Shader_Stages::PIXEL_STAGE;
			params[0].m_buffers = &m_appRenderer->m_material_uniform_buffer_list[material_index];

			uint32_t total_params_count = 1;
			
			Texture* diffuse_texture = cur_material_instance->GetDiffuseTexture();
			Texture* normal_texture = cur_material_instance->GetNormalTexture();
			Texture* height_texture = cur_material_instance->GetHeightTexture();
			Texture* specularTexture = cur_material_instance->GetSpecularTexture();
			Texture* metallicTexture = cur_material_instance->GetMetallicTexture();
			Texture* roughnessTexture = cur_material_instance->GetRoughnessTexture();

			++material_index;
			if ((mat_id & (uint32_t)MAT_ID_DIFFUSE_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 0;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &diffuse_texture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_NORMAL_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 1;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &normal_texture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_PARALLAX_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 2;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &height_texture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_SPECULAR_TEXUTERE) != 0)
			{
				params[total_params_count].m_binding_location = 3;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &specularTexture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_METALLIC_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 4;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &metallicTexture;
				++total_params_count;
			}
			if ((mat_id & (uint32_t)MAT_ID_ROUGHNESS_TEXTURE) != 0)
			{
				params[total_params_count].m_binding_location = 5;
				params[total_params_count].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
				params[total_params_count].m_shader_stages = Shader_Stages::PIXEL_STAGE;
				params[total_params_count].m_textures = &roughnessTexture;
				++total_params_count;
			}

			m_dxrenderer->cmd_bind_descriptor(pipeline, total_params_count, params);
			if (meshes_list.size() <= 0)
			{
				m_dxrenderer->cmd_draw_index(p_ref_model->GetIndexTotalCount(), 0, 0);
			}
			else
			{
				const Mesh& mesh_instance = meshes_list[mesh_index];
				m_dxrenderer->cmd_draw_index(mesh_instance.get_index_count(), mesh_instance.get_start_index(), mesh_instance.get_start_vertex());
			}
		}
	}

}


void AppRendererInstance::RenderSkybox()
{
	if (m_appRenderer->m_bakedSkyboxIrradianceInstanceList.empty())
	{
		return;
	}

	BufferUpdateDesc update_skybox_buffer_desc = {};
	update_skybox_buffer_desc.m_buffer = m_skyboxUniformBuffer;
	update_skybox_buffer_desc.m_pSource = &m_skyboxUniformData;
	update_skybox_buffer_desc.m_size = sizeof(SkyboxUniformData);
	m_dxrenderer->cmd_update_buffer(update_skybox_buffer_desc);

	m_dxrenderer->cmd_bind_pipeline(m_appRenderer->m_skybox_pipeline);
	m_dxrenderer->cmd_bind_vertex_buffer(m_appRenderer->m_skybox_vertices_buffer);

	DescriptorData params[3] = {};
	params[0].m_binding_location = 0;
	params[0].m_descriptor_type = DescriptorType::DESCRIPTOR_BUFFER;
	params[0].m_shader_stages = Shader_Stages::VERTEX_STAGE;
	params[0].m_buffers = &m_skyboxUniformBuffer;

	params[1].m_binding_location = 0;
	params[1].m_descriptor_type = DescriptorType::DESCRIPTOR_TEXTURE;
	params[1].m_shader_stages = Shader_Stages::PIXEL_STAGE;
	params[1].m_textures = &m_appRenderer->m_bakedSkyboxIrradianceInstanceList[0].m_pSkyboxTexture;

	params[2].m_binding_location = 0;
	params[2].m_descriptor_type = DescriptorType::DESCRIPTOR_SAMPLER;
	params[2].m_shader_stages = Shader_Stages::PIXEL_STAGE;
	params[2].m_samplers = &m_appRenderer->m_trillinear_sampler;

	m_dxrenderer->cmd_bind_descriptor(m_appRenderer->m_skybox_pipeline, 3, params);
	m_dxrenderer->cmd_draw(36, 0);
}