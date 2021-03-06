/**
 * @file Constraint.cpp
 * @author Aleksey Perfilev
 * @date 12/4/2019
 * @copyright Copyright(C) 2019 DigiPen Institute of Technology
 */
#include "Constraint.h"

MassMatrix::MassMatrix()
{
}

MassMatrix::MassMatrix(const Matrix& mass1, const Matrix& inertiaTensor1,
	const Matrix& mass2, const Matrix& inertiaTensor2) :
	m_mass1(mass1), m_inertiaTensor1(inertiaTensor1), m_mass2(mass2), m_inertiaTensor2(inertiaTensor2)
{
}
namespace hack
{
	inline Vector3 Transform(const Vector3& v, const Matrix& m)
	{
		using namespace DirectX;
		XMVECTOR v1 = XMLoadFloat3(&v);
		XMMATRIX M = XMLoadFloat4x4(&m);
		XMVECTOR X = XMVector3Transform(v1, M);

		Vector3 result;
		XMStoreFloat3(&result, X);
		return result;
	}
}
void MassMatrix::MultiplyByJacobian(Jacobian& result, const Jacobian& rhs)
{
	result.m_velocity1 = hack::Transform(rhs.m_velocity1, m_mass1);
	result.m_angularVelocity1 = hack::Transform(rhs.m_angularVelocity1, m_inertiaTensor1);
	result.m_velocity2 = hack::Transform(rhs.m_velocity2, m_mass2);
	result.m_angularVelocity2 = hack::Transform(rhs.m_angularVelocity2, m_inertiaTensor2);
}

Jacobian::Jacobian()
{
}

Jacobian::Jacobian(const Vector3& v1, const Vector3 w1, const Vector3 v2, const Vector3 w2) :
	m_velocity1(v1), m_angularVelocity1(w1), m_velocity2(v2), m_angularVelocity2(w2)
{
}

float Jacobian::operator*(const Jacobian& rhs)
{
	return m_velocity1.Dot(rhs.m_velocity1) +
		m_angularVelocity1.Dot(rhs.m_angularVelocity1) +
		m_velocity2.Dot(rhs.m_velocity2) +
		m_angularVelocity2.Dot(rhs.m_angularVelocity2);
}

void Jacobian::MultiplyByFloat(Jacobian& result, float rhs)
{
	result.m_velocity1 = m_velocity1 * rhs;
	result.m_angularVelocity1 = m_angularVelocity1 * rhs;
	result.m_velocity2 = m_velocity2 * rhs;
	result.m_angularVelocity2 = m_angularVelocity2 * rhs;
}

void Jacobian::AddJacobian(Jacobian& result, const Jacobian& rhs)
{
	result.m_velocity1 = m_velocity1 + rhs.m_velocity1;
	result.m_angularVelocity1 = m_angularVelocity1 + rhs.m_angularVelocity1;
	result.m_velocity2 = m_velocity2 + rhs.m_velocity2;
	result.m_angularVelocity2 = m_angularVelocity2 + rhs.m_angularVelocity2;
}

Constraint::Constraint() : m_object1(nullptr), m_object2(nullptr), m_bias(0.0f), m_lambda(0.0f), m_depthPen(0.0f)
{
}

Constraint::Constraint(RigidbodyComponent* obj1, RigidbodyComponent* obj2, float depth) : m_object1(obj1), m_object2(obj2), m_depthPen(depth)
{
}

Constraint::~Constraint()
{
}

void Constraint::CalculateNormalJacobian(const Vector3& normal, const Vector3& pointA, const Vector3& pointB)
{
	m_normal = normal;
	m_normal.Normalize();
	Vector3 r1 = pointA - m_object1->GetPosition();
	Vector3 r2 = pointB - m_object2->GetPosition();

	CalculateJacobian(m_normal, r1, r2);
}

void Constraint::CalculateFrictionJacobians(const Vector3& normal, const Vector3& pointA, const Vector3& pointB, Constraint& constraintFriction1, Constraint& constraintFriction2)
{
	// calculate the tangent vectors to normal to simulate both frictions
	Vector3 normalNormalized = normal;
	normalNormalized.Normalize();
	constraintFriction1.m_normal = normal;
	constraintFriction2.m_normal = normal;

	Vector3 u1; // tangent to normal, direction unit vector for friction in 1 of the directions
	Vector3 u2; // bitangent to normal, direction unit vector for friction in 2 of the directions
	MathUtil::BuildTangentBitangent(normal, u1, u2);

	Vector3 r1 = pointA - constraintFriction1.m_object1->GetPosition();
	Vector3 r2 = pointB - constraintFriction1.m_object2->GetPosition();

	constraintFriction1.CalculateJacobian(u1, r1, r2);
	constraintFriction2.CalculateJacobian(u2, r1, r2);
}

void Constraint::ToMatrix4(Matrix& result, const Jacobian& jacobian)
{
	result = Matrix(Vector4(jacobian.m_velocity1.x, jacobian.m_velocity1.y, jacobian.m_velocity1.z, 0), 
					Vector4(jacobian.m_angularVelocity1.x, jacobian.m_angularVelocity1.y, jacobian.m_angularVelocity1.z, 0), 
					Vector4(jacobian.m_velocity2.x, jacobian.m_velocity2.y, jacobian.m_velocity2.z, 0), 
					Vector4(jacobian.m_angularVelocity2.x, jacobian.m_angularVelocity2.y, jacobian.m_angularVelocity2.z, 1));
}

void Constraint::CalculateJacobian(const Vector3& direction, const Vector3& radiusVector1, const Vector3& radiusVector2)
{
	const Vector3 r1CrossDir = radiusVector1.Cross(direction);
	const Vector3 r2CrossDir = radiusVector2.Cross(direction);

	m_jacobian.m_velocity1 = -direction;
	m_jacobian.m_angularVelocity1 = -r1CrossDir;
	m_jacobian.m_velocity2 = direction;
	m_jacobian.m_angularVelocity2 = r2CrossDir;
}

Contact::Contact() : m_objectA(nullptr), m_objectB(nullptr), m_depth(0.0f)
{

}

void Contact::BuildConstraints()
{
	m_constraints.clear();
	// Normal constraint for both objects
	Constraint constraintNormal(m_objectA, m_objectB, m_depth);
	constraintNormal.CalculateNormalJacobian(m_normal, m_pA, m_pB);
	m_constraints.push_back(constraintNormal);

	// Friction constraint for both objects
	Constraint constraintFriction1(m_objectA, m_objectB);
	Constraint constraintFriction2(m_objectA, m_objectB);
	Constraint::CalculateFrictionJacobians(m_normal, m_pA, m_pB, constraintFriction1, constraintFriction2);
	m_constraints.push_back(constraintFriction1);
	m_constraints.push_back(constraintFriction2);

}

ContactManifold::ContactManifold(Contact& contact) : m_object1(contact.m_objectA), m_object2(contact.m_objectB)
{
	m_contacts.reserve(4);
	ProccessCollision(contact);
}

void ContactManifold::ProccessCollision(Contact& newContact)
{
	bool isKeepContact = true;
	for (Contact& contact : m_contacts)
	{
		const Vector3 rA = newContact.m_pA - contact.m_pA;
		const bool rAFarEnough = rA.LengthSquared() > PhysicsUtils::Consts::penetrationEpsilon;
		if (!rAFarEnough)
		{
			isKeepContact = false;
			break;
		}
	}
	if (isKeepContact)
	{
		newContact.BuildConstraints();
		m_contacts.push_back(newContact);
	}
}

void ContactManifold::KeepOnlyFourContacts()
{
	if (m_contacts.size() <= 4)
	{
		return;
	}

	// find the deepest penetrating one
	Contact * deepest = nullptr;
	float penetration = -FLT_MAX;
	for (Contact& contact : m_contacts)
	{
		if (contact.m_depth > penetration)
		{
			penetration = contact.m_depth;
			deepest = &contact;
		}
	}

	// find second contact
	Contact* furthest1 = nullptr;
	float distanceSq1 = -FLT_MAX;
	for (Contact& contact : m_contacts)
	{
		const float distSq = (contact.m_pA - deepest->m_pA).LengthSquared();
		if (distSq > distanceSq1)
		{
			distanceSq1 = distSq;
			furthest1 = &contact;
		}
	}

	// find third contact
	Contact* furthest2 = nullptr;
	float distanceSq2 = -FLT_MAX;
	for (Contact& contact : m_contacts)
	{
		float u, v;
		PhysicsUtils::BarycentricCoordinates(contact.m_pA, deepest->m_pA, furthest1->m_pA , u, v);
		const Vector3 closestPoint = u * deepest->m_pA + v * furthest1->m_pA;
		const float distSq = (contact.m_pA - closestPoint).LengthSquared();
		if (distSq > distanceSq2)
		{
			distanceSq2 = distSq;
			furthest2 = &contact;
		}
	}

	// find fourth contact
	Contact* furthest3 = nullptr;
	float distanceSq3 = -FLT_MAX;
	for (Contact& contact : m_contacts)
	{
		float u, v, w;
		PhysicsUtils::BarycentricCoordinates(contact.m_pA, deepest->m_pA, furthest1->m_pA, furthest2->m_pA, u, v, w);
		const Vector3 closestPoint = u * deepest->m_pA + v * furthest1->m_pA + w * furthest2->m_pA;

		const float distSq = (contact.m_pA - closestPoint).LengthSquared();
		if (distSq > distanceSq3)
		{
			distanceSq3 = distSq;
			furthest3 = &contact;
		}
	}
	float u, v, w;
	PhysicsUtils::BarycentricCoordinates(furthest3->m_pA, deepest->m_pA, furthest1->m_pA, furthest2->m_pA, u, v, w);
	// if outside of the triangle add the new contact
	if (u < 0.0f || v < 0.0f || w < 0.0f)
	{
		m_contacts.clear();
		m_contacts.push_back(*deepest);
		m_contacts.push_back(*furthest1);
		m_contacts.push_back(*furthest2);
		m_contacts.push_back(*furthest3);
	}
}