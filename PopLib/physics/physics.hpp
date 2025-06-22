/* Sexy Chipmunk, a physics engine for the PopCap Games Framework using Scott Lembcke's excellent chipmunk physics
 * library */
/* Copyright (c) 2007-2008 W.P. van Paassen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __PHYSICS_HPP__
#define __PHYSICS_HPP__
#ifdef _WIN32
#pragma once
#endif

#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include "chipmunk/chipmunk.h"
#include "physicslistener.hpp"
#include "math/vector.hpp"

namespace PopLib
{
class CollisionPoint;
class PhysicsObject;
class Joint;

class Physics
{
	friend class PhysicsObject;

  public:
	Physics();
	~Physics();

	void Init();

	bool IsInitialized()
	{
		return space != 0;
	}

	void SetGravity(const Vector2 &gravity);
	void SetDamping(cpFloat damping);
	void SetIterations(int iter);
	void ResizeStaticHash(float dimension, int count);
	void ResizeActiveHash(float dimension, int count);

	void Update();
	void Draw(Graphics *g);
	void Clear();

	void SetSteps(int steps);

	void SetDelta(float delta)
	{
		this->delta = delta;
	}

	PhysicsObject *CreateObject(cpFloat mass, cpFloat inertia);
	PhysicsObject *CreateStaticObject();
	void DestroyObject(PhysicsObject *object, bool erase = true);
	bool IsValidObject(PhysicsObject *object) const;

	void SetPhysicsListener(PhysicsListener *p)
	{
		listener = p;
	}

	void RegisterCollisionType(uint32_t type_a, uint32_t type_b = 0);
	void UnregisterCollisionType(uint32_t type_a, uint32_t type_b = 0);

	std::vector<PhysicsObject *> &GetPhysicsObjects()
	{
		return objects;
	}

	// help functions

	void ApplySpringForce(PhysicsObject *obj1, PhysicsObject *obj2, const Vector2 &anchor1, const Vector2 &anchor2,
						  float rest_length, float spring, float damping);

	Joint CreatePinJoint(const PhysicsObject *obj1, const PhysicsObject *obj2, const Vector2 &anchor1,
						 const Vector2 &anchor2);
	Joint CreateSlideJoint(const PhysicsObject *obj1, const PhysicsObject *obj2, const Vector2 &anchor1,
						   const Vector2 &anchor2, float min, float max);
	Joint CreatePivotJoint(const PhysicsObject *obj1, const PhysicsObject *obj2, const Vector2 &pivot);
	void RemoveJoint(const Joint &joint);
	void RemoveJoint(const PhysicsObject *obj1, const PhysicsObject *obj2);
	void RemoveJoints(const PhysicsObject *obj);
	bool IsJoined(const PhysicsObject *obj1, const PhysicsObject *obj2) const;
	std::vector<std::pair<Vector2, Vector2>> GetJoints(const PhysicsObject *obj1, const PhysicsObject *obj2) const;
	std::vector<std::pair<Vector2, Vector2>> GetJoints(const PhysicsObject *obj1) const;
	std::vector<std::pair<Vector2, Vector2>> GetJoints() const;
	std::set<PhysicsObject *> GetJoinedPhysicsObjects(const PhysicsObject *obj1) const;

	static cpFloat ComputeMomentForPoly(cpFloat moment, int numVerts, Vector2 *vectors, const Vector2 &offset)
	{
		return cpMomentForPoly(moment, numVerts, (cpVect *)vectors, cpv(offset.x, offset.y));
	}

	static cpFloat ComputeMomentForCircle(cpFloat moment, cpFloat r1, cpFloat r2, const Vector2 &offset)
	{
		return cpMomentForCircle(moment, r1, r2, cpv(offset.x, offset.y));
	}

	static Vector2 RotateVector(const Vector2 &v1, const Vector2 &v2)
	{
		cpVect r = cpvrotate(cpv(v1.x, v1.y), cpv(v2.x, v2.y));
		return Vector2(r.x, r.y);
	}

	static Vector2 SumCollisionImpulses(int numContacts, CollisionPoint *contacts);
	static Vector2 SumCollisionImpulsesWithFriction(int numContacts, CollisionPoint *contacts);

  private:
	cpSpace *space;
	int steps;
	cpFloat delta;
	std::vector<PhysicsObject *> objects;
	std::map<cpBody *, PhysicsObject *> body_to_object;
	std::vector<cpJoint *> joints;
	PhysicsListener *listener;

	void AddUniqueJoint(std::vector<std::pair<Vector2, Vector2>> *v, const Vector2 &start, const Vector2 &end) const;
	const std::vector<cpJoint *> GetJointsOfObject(const PhysicsObject *obj) const;
	void RemoveJoint(const cpJoint *joint);

	static void AllCollisions(void *ptr, void *data);
	static void HashQuery(void *ptr, void *data);
	static int CollFunc(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data);
	PhysicsObject *findObjectByBody(cpBody *body) const;
	PhysicsObject *FindObject(cpBody *body, cpShape *shape);
	PhysicsObject *FindObject(cpShape *shape);

	typedef struct typed_data
	{
		Graphics *graphics;
		Physics *physics;
	} TypedData;
};

class PhysicsObject
{
  private:
	explicit PhysicsObject(cpFloat mass, cpFloat inertia, Physics *physics, bool is_static = false);
	~PhysicsObject();

	friend class Physics;
	friend class CollisionObject;

	cpBody *body;
	std::vector<cpShape *> shapes;
	Physics *physics;
	int colliding_shape_index;

  public:
	bool is_static;

	// body functions

	void SetMass(cpFloat m)
	{
		cpBodySetMass(body, m);
	}

	void SetMoment(cpFloat i)
	{
		cpBodySetMoment(body, i);
	}

	void SetAngle(cpFloat a)
	{
		cpBodySetAngle(body, a);
	}

	void ResetForces()
	{
		cpBodyResetForces(body);
	}
	void SetAngularVelocity(cpFloat w);
	void SetVelocity(const Vector2 &v);

	void SetPosition(const Vector2 &p)
	{
		body->p = cpv(p.x, p.y);
	}
	void UpdatePosition();
	void UpdateVelocity();

	void ApplyImpulse(const Vector2 &j, const Vector2 &r)
	{
		cpBodyApplyImpulse(body, cpv(j.x, j.y), cpv(r.x, r.y));
	}

	void ApplyForce(const Vector2 &f, const Vector2 &r)
	{
		cpBodyApplyForce(body, cpv(f.x, f.y), cpv(r.x, r.y));
	}
	cpBody *GetBody() const
	{
		return body;
	}
	float GetAngle() const
	{
		return (float)body->a;
	}
	Vector2 GetRotation() const
	{
		return Vector2(body->rot.x, body->rot.y);
	}
	Vector2 GetPosition() const
	{
		return Vector2(body->p.x, body->p.y);
	}
	float GetPosX() const
	{
		return body->p.x;
	}
	float GetPosY() const
	{
		return body->p.y;
	}
	Vector2 GetVelocity() const
	{
		return Vector2(body->v.x, body->v.y);
	}
	float GetVeloX() const
	{
		return body->v.x;
	}
	float GetVeloY() const
	{
		return body->v.y;
	}

	// shape functions

	void AddCircleShape(cpFloat radius, const Vector2 &offset, cpFloat elasticity, cpFloat friction);
	void AddSegmentShape(const Vector2 &begin, const Vector2 &end, cpFloat radius, cpFloat elasticity,
						 cpFloat friction);
	void AddPolyShape(int numVerts, Vector2 *vectors, const Vector2 &offset, cpFloat elasticity, cpFloat friction);
	void SetCollisionType(unsigned int type, int shape_index = 0);
	void SetGroup(unsigned int group, int shape_index = 0);
	void SetLayers(unsigned int layers, int shape_index = 0);
	void SetData(void *data, int shape_index = 0);
	unsigned int GetCollisionType(int shape_index = 0) const;
	unsigned int GetGroup(int shape_index = 0) const;
	unsigned int GetLayers(int shape_index = 0) const;
	void *GetData(int shape_index = 0) const;
	int GetNumberVertices(int shape_index = 0) const;
	Vector2 GetVertex(int vertex_index, int shape_index = 0) const;
	Vector2 GetSegmentShapeBegin(int shape_index = 0) const;
	Vector2 GetSegmentShapeEnd(int shape_index = 0) const;
	float GetSegmentShapeRadius(int shape_index = 0) const;
	float GetCircleShapeRadius(int shape_index = 0) const;
	Vector2 GetCircleShapeCenter(int shape_index = 0) const;
	Vector2 GetCircleShapeOffset(int shape_index = 0) const;
	int GetShapeType(int shape_index = 0) const;
	int GetNumberOfShapes() const;
	int GetCollidingShapeIndex() const;
	void RemoveShape(int shape_index = 0);
	float GetFriction(int shape_index = 0) const;
	float GetElasticity(int shape_index = 0) const;

	enum SHAPE_TYPE
	{
		CIRCLE_SHAPE = CP_CIRCLE_SHAPE,
		SEGMENT_SHAPE,
		POLY_SHAPE,
		NR_SHAPE_TYPES
	};
};

class CollisionPoint
{
  public:
	CollisionPoint(const Vector2 &point, const Vector2 &normal, float distance)
		: point(point), normal(normal), distance(distance)
	{
	}

	~CollisionPoint()
	{
	}

	Vector2 point;
	Vector2 normal;
	float distance;

	// Calculated by cpArbiterPreStep().
	Vector2 r1, r2;
	float nMass, tMass, bounce;
	// Persistant contact information.
	float jnAcc, jtAcc, jBias;
	float bias;

	// Hash value used to (mostly) uniquely identify a contact.
	uint32_t hash;
};

class CollisionObject
{
  public:
	CollisionObject(PhysicsObject *object1, PhysicsObject *object2, const CollisionPoint *points, int num_points,
					float normal_coef = 1.0f)
		: object1(object1), object2(object2), points(points), num_points(num_points), normal_coef(normal_coef)
	{
	}

	~CollisionObject()
	{
	}

	PhysicsObject *object1;
	PhysicsObject *object2;
	const CollisionPoint *points;
	int num_points;
	float normal_coef;
};

class Joint
{
  private:
	Joint(cpJoint *joint, PhysicsObject *obj1, PhysicsObject *obj2, const Vector2 &anchor1, const Vector2 &anchor2)
		: joint(joint), object1(obj1), object2(obj2), anchor1(anchor1), anchor2(anchor2)
	{
	}

	Joint(cpJoint *joint, PhysicsObject *obj1, PhysicsObject *obj2, const Vector2 &pivot)
		: joint(joint), object1(obj1), object2(obj2), pivot(pivot)
	{
	}

	friend class Physics;

	cpJoint *joint;
	PhysicsObject *object1;
	PhysicsObject *object2;
	Vector2 anchor1;
	Vector2 anchor2;
	Vector2 pivot;

  public:
	Joint()
	{
	}
	~Joint()
	{
	}

	const PhysicsObject *GetPhysicsObject1()
	{
		return object1;
	}

	const PhysicsObject *GetPhysicsObject2()
	{
		return object2;
	}

	const Vector2 *GetAnchor1()
	{
		if (joint->type == CP_PIN_JOINT || joint->type == CP_SLIDE_JOINT)
			return &anchor1;
		return NULL;
	}

	const Vector2 *GetAnchor2()
	{
		if (joint->type == CP_PIN_JOINT || joint->type == CP_SLIDE_JOINT)
			return &anchor2;
		return NULL;
	}

	const Vector2 *GetPivot()
	{
		if (joint->type == CP_PIVOT_JOINT)
			return &pivot;
		return NULL;
	}

	float GetMinOfSlide() const
	{
		assert(joint->type == CP_SLIDE_JOINT);
		return ((cpSlideJoint *)joint)->min;
	}

	float GetMaxOfSlide() const
	{
		assert(joint->type == CP_SLIDE_JOINT);
		return ((cpSlideJoint *)joint)->max;
	}
	std::pair<Vector2, Vector2> GetPoints() const;
};

} // namespace PopLib

#endif