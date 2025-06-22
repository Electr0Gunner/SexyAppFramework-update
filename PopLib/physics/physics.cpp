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

#include "physics.hpp"
#include <assert.h>
#include <cstdlib>
#include <set>
#include <map>
#include <algorithm>
#include <utility>
#include "graphics/graphics.hpp"

/* TODO
 * inline
 * copy constructor, assignment operator
 */

using namespace PopLib;

Physics::Physics() : space(NULL), steps(1), delta(0.0f), objects(), body_to_object(), joints(), listener(NULL)
{
	cpInitChipmunk();
}

Physics::~Physics()
{
	Clear();
	cpSpaceFreeChildren(space);
	cpSpaceFree(space);
	space = NULL;
}

void Physics::Init()
{
	if (space == NULL)
	{
		cpResetShapeIdCounter();
		space = cpSpaceNew();
		assert(space != NULL);

		cpSpaceSetDefaultCollisionPairFunc(space, NULL, NULL);

		space->gravity = cpv(0, 100);

		delta = 1.0f / 60.0f / (cpFloat)steps;
	}
}

void Physics::SetSteps(int steps)
{
	this->steps = steps;
	delta = 1.0f / 60.0f / (cpFloat)steps;
}

int Physics::CollFunc(cpShape *a, cpShape *b, cpContact *contacts, int numContacts, cpFloat normal_coef, void *data)
{
	assert(data != NULL);

	TypedData *t_data = reinterpret_cast<TypedData *>(data);

	PhysicsObject *obj1 = t_data->physics->FindObject(a->body, a);
	PhysicsObject *obj2 = t_data->physics->FindObject(b->body, b);

	assert(obj1 != NULL);
	assert(obj2 != NULL);
	assert(sizeof(CollisionPoint) == sizeof(cpContact));

	CollisionObject col(obj1, obj2, reinterpret_cast<CollisionPoint *>(contacts), numContacts, normal_coef);

	return t_data->physics->listener->HandleTypedCollision(&col);
}

Vector2 Physics::SumCollisionImpulses(int numContacts, CollisionPoint *contacts)
{
	assert(sizeof(CollisionPoint) == sizeof(cpContact));
	cpVect sum_impulse = cpContactsSumImpulses((cpContact *)contacts, numContacts);
	return Vector2(sum_impulse.x, sum_impulse.y);
}

Vector2 Physics::SumCollisionImpulsesWithFriction(int numContacts, CollisionPoint *contacts)
{
	assert(sizeof(CollisionPoint) == sizeof(cpContact));
	cpVect sum_impulse = cpContactsSumImpulsesWithFriction(reinterpret_cast<cpContact *>(contacts), numContacts);
	return Vector2(sum_impulse.x, sum_impulse.y);
}

void Physics::AllCollisions(void *ptr, void *data)
{
	assert(data != NULL && ptr != NULL);

	cpArbiter *arb = reinterpret_cast<cpArbiter *>(ptr);

	Physics *p = reinterpret_cast<Physics *>(data);

	PhysicsObject *obj1 = p->FindObject(arb->a->body, arb->a);
	PhysicsObject *obj2 = p->FindObject(arb->b->body, arb->b);

	assert(obj1 != NULL);
	assert(obj2 != NULL);
	assert(sizeof(CollisionPoint) == sizeof(cpContact));

	CollisionObject col(obj1, obj2, reinterpret_cast<CollisionPoint *>(arb->contacts), arb->numContacts);

	p->listener->HandleCollision(&col);
}

void Physics::HashQuery(void *ptr, void *data)
{
	assert(ptr != NULL && data != NULL);

	TypedData *t_data = reinterpret_cast<TypedData *>(data);

	cpShape *shape = reinterpret_cast<cpShape *>(ptr);
	PhysicsObject *obj = t_data->physics->FindObject(shape);
	assert(obj != NULL);

	t_data->physics->listener->DrawPhysicsObject(obj, t_data->graphics);
}

void Physics::Update()
{
	{
		assert(listener != NULL);
		for (int i = 0; i < steps; i++)
		{
			listener->BeforePhysicsStep();
			cpSpaceStep(space, delta);
			listener->AfterPhysicsStep();

			cpArrayEach(space->arbiters, &AllCollisions, this);
		}
	}
}

void Physics::Draw(Graphics *g)
{
	TypedData data;
	data.graphics = g;
	data.physics = this;

	cpSpaceHashEach(space->activeShapes, &HashQuery, reinterpret_cast<void *>(&data));
	cpSpaceHashEach(space->staticShapes, &HashQuery, reinterpret_cast<void *>(&data));
}

void Physics::SetGravity(const Vector2 &gravity)
{
	assert(space != NULL);
	space->gravity = cpv(gravity.x, gravity.y);
}

void Physics::SetDamping(cpFloat damping)
{
	assert(space != NULL);
	space->damping = damping;
}

void Physics::SetIterations(int iter)
{
	assert(space != NULL);
	space->iterations = iter;
}

void Physics::ResizeStaticHash(float dimension, int count)
{
	assert(space != NULL);
	cpSpaceResizeStaticHash(space, dimension, count);
}

void Physics::ResizeActiveHash(float dimension, int count)
{
	assert(space != NULL);
	cpSpaceResizeActiveHash(space, dimension, count);
}

void Physics::Clear()
{
	std::vector<PhysicsObject *>::iterator it = objects.begin();
	while (it != objects.end())
	{
		// The false means it won't be removed from the vector. It would mess up our iteratiom, I guess.
		DestroyObject(*it, false);
		++it;
	}

	body_to_object.clear();
	objects.clear();
	joints.clear();
}

PhysicsObject *Physics::CreateObject(cpFloat mass, cpFloat inertia)
{
	assert(IsInitialized());
	PhysicsObject *obj = new PhysicsObject(mass, inertia, this);
	objects.push_back(obj);
	cpBody *body = obj->GetBody();
	if (body)
	{
		assert(!findObjectByBody(body));
		body_to_object[body] = obj;
	}
	return obj;
}

PhysicsObject *Physics::CreateStaticObject()
{
	PhysicsObject *obj = new PhysicsObject(INFINITY, INFINITY, this, true);
	objects.push_back(obj);
	cpBody *body = obj->GetBody();
	if (body)
	{
		assert(!findObjectByBody(body));
		body_to_object[body] = obj;
	}
	return obj;
}

void Physics::DestroyObject(PhysicsObject *object, bool erase)
{
	assert(IsValidObject(object));

	if (!object->shapes.empty())
	{
		if (object->is_static)
		{
			std::vector<cpShape *>::iterator it = object->shapes.begin();
			while (it != object->shapes.end())
			{
				cpSpaceRemoveStaticShape(space, *it);
				++it;
			}
		}
		else
		{
			std::vector<cpShape *>::iterator it = object->shapes.begin();
			while (it != object->shapes.end())
			{
				cpSpaceRemoveShape(space, *it);
				++it;
			}
		}
	}

	if (erase)
	{
		// Let's assume the body_to_object will be deleted soon, by the caller.
		std::map<cpBody *, PhysicsObject *>::iterator it = body_to_object.find(object->body);
		if (it != body_to_object.end())
		{
			body_to_object.erase(it);
		}
	}

	if (!object->is_static)
	{
		cpSpaceRemoveBody(space, object->body);
	}
	else
	{
		cpSpaceRehashStatic(space);
	}

	std::vector<cpJoint *> j = GetJointsOfObject(object);
	std::vector<cpJoint *>::iterator it = j.begin();
	while (it != j.end())
	{
		RemoveJoint(*it);
		++it;
	}

	cpBodyFree(object->body);
	std::vector<cpShape *>::iterator sit = object->shapes.begin();
	while (sit != object->shapes.end())
	{
		cpShapeFree(*sit);
		++sit;
	}

	if (erase)
	{
		// If it is part of the "objects", delete and remove it.
		std::vector<PhysicsObject *>::iterator pit = std::find(objects.begin(), objects.end(), object);
		if (pit != objects.end())
		{
			delete (*pit);
			objects.erase(pit);
		}
	}
	// ???? Doing this will cause a crash in ~WP_Sprite() object->body = NULL;
}

bool Physics::IsValidObject(PhysicsObject *object) const
{
	std::vector<PhysicsObject *>::const_iterator it = std::find(objects.begin(), objects.end(), object);
	return (it != objects.end());
}

void Physics::RegisterCollisionType(uint32_t type_a, uint32_t type_b)
{
	TypedData *data = new TypedData;
	data->graphics = NULL; // ???? no graphics, hmmm
	data->physics = this;
	cpSpaceAddCollisionPairFunc(space, type_a, type_b, (cpCollFunc)&CollFunc, reinterpret_cast<void *>(data));
}

void Physics::UnregisterCollisionType(uint32_t type_a, uint32_t type_b)
{
	unsigned int ids[] = {type_a, type_b};
	unsigned int hash = CP_HASH_PAIR(type_a, type_b);
	cpCollPairFunc *old_pair = static_cast<cpCollPairFunc *>(cpHashSetFind(space->collFuncSet, hash, ids));
	delete reinterpret_cast<TypedData *>(old_pair->data);
	cpSpaceRemoveCollisionPairFunc(space, type_a, type_b);
}

void Physics::ApplySpringForce(PhysicsObject *obj1, PhysicsObject *obj2, const Vector2 &anchor1, const Vector2 &anchor2,
							   float rest_length, float spring, float damping)
{
	cpDampedSpring(obj1->body, obj2->body, cpv(anchor1.x, anchor1.y), cpv(anchor2.x, anchor2.y), rest_length, spring,
				   damping, delta);
}

Joint Physics::CreatePinJoint(const PhysicsObject *obj1, const PhysicsObject *obj2, const Vector2 &anchor1,
							  const Vector2 &anchor2)
{
	cpJoint *joint = cpPinJointNew(obj1->body, obj2->body, cpv(anchor1.x, anchor1.y), cpv(anchor2.x, anchor2.y));
	joints.push_back(joint);
	cpSpaceAddJoint(space, joint);
	return Joint(joint, const_cast<PhysicsObject *>(obj1), const_cast<PhysicsObject *>(obj2), anchor1, anchor2);
}

Joint Physics::CreateSlideJoint(const PhysicsObject *obj1, const PhysicsObject *obj2, const Vector2 &anchor1,
								const Vector2 &anchor2, float min, float max)
{
	cpJoint *joint =
		cpSlideJointNew(obj1->body, obj2->body, cpv(anchor1.x, anchor1.y), cpv(anchor2.x, anchor2.y), min, max);
	joints.push_back(joint);
	cpSpaceAddJoint(space, joint);
	return Joint(joint, const_cast<PhysicsObject *>(obj1), const_cast<PhysicsObject *>(obj2), anchor1, anchor2);
}

Joint Physics::CreatePivotJoint(const PhysicsObject *obj1, const PhysicsObject *obj2, const Vector2 &pivot)
{
	cpJoint *joint = cpPivotJointNew(obj1->body, obj2->body, cpv(pivot.x, pivot.y));
	joints.push_back(joint);
	cpSpaceAddJoint(space, joint);
	return Joint(joint, const_cast<PhysicsObject *>(obj1), const_cast<PhysicsObject *>(obj2), pivot);
}

std::vector<std::pair<Vector2, Vector2>> Physics::GetJoints() const
{
	std::vector<cpJoint *>::const_iterator it = joints.begin();
	std::vector<std::pair<Vector2, Vector2>> v;

	while (it != joints.end())
	{
		cpVect pos1 = (*it)->a->p;
		cpVect pos2 = (*it)->b->p;
		cpVect s, e, v1, v2;
		switch ((*it)->type)
		{
		case CP_PIN_JOINT:
			v1 = ((cpPinJoint *)(*it))->anchr1;
			s = cpvadd(pos1, cpvrotate((*it)->a->rot, v1));
			v2 = ((cpPinJoint *)(*it))->anchr2;
			e = cpvadd(pos2, cpvrotate((*it)->b->rot, v2));
			break;
		case CP_SLIDE_JOINT:
			v1 = ((cpSlideJoint *)(*it))->anchr1;
			s = cpvadd(pos1, cpvrotate((*it)->a->rot, v1));
			v2 = ((cpSlideJoint *)(*it))->anchr2;
			e = cpvadd(pos2, cpvrotate((*it)->b->rot, v2));
			break;
		case CP_PIVOT_JOINT:
			v1 = ((cpPivotJoint *)(*it))->anchr1;
			s = pos1; // cpvadd(pos1, cpvrotate(v1, (*it)->a->rot));
			v2 = ((cpPivotJoint *)(*it))->anchr2;
			e = pos2; // cpvadd(pos2, cpvrotate(v2, (*it)->b->rot));
			break;
		case CP_GROOVE_JOINT:
			// TODO. What to do with these?
			assert(0);
			break;
		case CP_NUM_JOINTS:
		default:
			// This should never happen.
			assert(0);
			break;
		}
		AddUniqueJoint(&v, Vector2(s.x, s.y), Vector2(e.x, e.y));
		++it;
	}
	return v;
}

std::vector<std::pair<Vector2, Vector2>> Physics::GetJoints(const PhysicsObject *obj1, const PhysicsObject *obj2) const
{

	const std::vector<cpJoint *> j = GetJointsOfObject(obj1);
	std::vector<std::pair<Vector2, Vector2>> v;

	std::vector<cpJoint *>::const_iterator it = j.begin();
	while (it != j.end())
	{
		if (((*it)->a == obj1->body || (*it)->b == obj1->body) && ((*it)->a == obj2->body || (*it)->b == obj2->body))
		{

			Vector2 start = obj1->GetPosition();
			Vector2 end = obj2->GetPosition();
			cpVect v1, v2;

			switch ((*it)->type)
			{
			case CP_PIN_JOINT:
				v1 = ((cpPinJoint *)(*it))->anchr1;
				v2 = ((cpPinJoint *)(*it))->anchr2;
				if ((*it)->a == obj1->body)
				{
					cpVect rot = cpvrotate((*it)->a->rot, v1);
					start += Vector2(rot.x, rot.y);
					rot = cpvrotate((*it)->b->rot, v2);
					end += Vector2(rot.x, rot.y);
				}
				else
				{
					cpVect rot = cpvrotate((*it)->b->rot, v1);
					start += Vector2(rot.x, rot.y);
					rot = cpvrotate((*it)->a->rot, v2);
					end += Vector2(rot.x, rot.y);
				}
				break;
			case CP_SLIDE_JOINT:
				v1 = ((cpSlideJoint *)(*it))->anchr1;
				v2 = ((cpSlideJoint *)(*it))->anchr2;
				if ((*it)->a == obj1->body)
				{
					cpVect rot = cpvrotate((*it)->a->rot, v1);
					start += Vector2(rot.x, rot.y);
					rot = cpvrotate((*it)->b->rot, v2);
					end += Vector2(rot.x, rot.y);
				}
				else
				{
					cpVect rot = cpvrotate((*it)->b->rot, v1);
					start += Vector2(rot.x, rot.y);
					rot = cpvrotate((*it)->a->rot, v2);
					end += Vector2(rot.x, rot.y);
				}
				break;
			case CP_PIVOT_JOINT:
				// ???? Why skip these?
				break;
			case CP_GROOVE_JOINT:
				// TODO. What to do with these?
				assert(0);
				break;
			case CP_NUM_JOINTS:
			default:
				// This should never happen.
				assert(0);
				break;
			}
			AddUniqueJoint(&v, start, end);
		}
		++it;
	}
	return v;
}

std::vector<std::pair<Vector2, Vector2>> Physics::GetJoints(const PhysicsObject *obj1) const
{

	const std::vector<cpJoint *> j = GetJointsOfObject(obj1);
	std::vector<std::pair<Vector2, Vector2>> v;

	std::vector<cpJoint *>::const_iterator it = j.begin();
	while (it != j.end())
	{
		Vector2 start = obj1->GetPosition();
		Vector2 end;
		if ((*it)->a == obj1->body)
		{
			cpVect v = (*it)->b->p;
			end = Vector2(v.x, v.y);
		}
		else
		{
			cpVect v = (*it)->a->p;
			end = Vector2(v.x, v.y);
		}
		cpVect v1, v2;
		switch ((*it)->type)
		{
		case CP_PIN_JOINT:
			v1 = ((cpPinJoint *)(*it))->anchr1;
			v2 = ((cpPinJoint *)(*it))->anchr2;

			if ((*it)->a == obj1->body)
			{
				cpVect rot = cpvrotate((*it)->a->rot, v1);
				start += Vector2(rot.x, rot.y);
				rot = cpvrotate((*it)->b->rot, v2);
				end += Vector2(rot.x, rot.y);
			}
			else
			{
				cpVect rot = cpvrotate((*it)->b->rot, v1);
				start += Vector2(rot.x, rot.y);
				rot = cpvrotate((*it)->a->rot, v2);
				end += Vector2(rot.x, rot.y);
			}
			break;
		case CP_SLIDE_JOINT:
			v1 = ((cpSlideJoint *)(*it))->anchr1;
			v2 = ((cpSlideJoint *)(*it))->anchr2;
			if ((*it)->a == obj1->body)
			{
				cpVect rot = cpvrotate((*it)->a->rot, v1);
				start += Vector2(rot.x, rot.y);
				rot = cpvrotate((*it)->b->rot, v2);
				end += Vector2(rot.x, rot.y);
			}
			else
			{
				cpVect rot = cpvrotate((*it)->b->rot, v1);
				start += Vector2(rot.x, rot.y);
				rot = cpvrotate((*it)->a->rot, v2);
				end += Vector2(rot.x, rot.y);
			}
			break;
		case CP_PIVOT_JOINT:
			// ????
			break;
		case CP_GROOVE_JOINT:
			// TODO. What to do with these?
			assert(0);
			break;
		case CP_NUM_JOINTS:
		default:
			// This should never happen.
			assert(0);
			break;
		}
		AddUniqueJoint(&v, start, end);
		++it;
	}
	return v;
}

void Physics::AddUniqueJoint(std::vector<std::pair<Vector2, Vector2>> *v, const Vector2 &start,
							 const Vector2 &end) const
{
	std::vector<std::pair<Vector2, Vector2>>::iterator vit = v->begin();
	while (vit != v->end())
	{
		if (((*vit).first == start && (*vit).second == end) || ((*vit).first == end && (*vit).second == start))
		{
			return;
		}
		++vit;
	}
	v->push_back(std::make_pair(start, end));
}

bool Physics::IsJoined(const PhysicsObject *obj1, const PhysicsObject *obj2) const
{
	std::vector<cpJoint *>::const_iterator it = joints.begin();
	while (it != joints.end())
	{
		if (((*it)->a == obj1->body || (*it)->b == obj1->body) && ((*it)->a == obj2->body || (*it)->b == obj2->body))
		{
			return true;
		}
		++it;
	}
	return false;
}

void Physics::RemoveJoint(const PhysicsObject *obj1, const PhysicsObject *obj2)
{
	assert(obj1->body != NULL && obj2->body != NULL);
	std::vector<cpJoint *>::iterator it = joints.begin();
	while (it != joints.end())
	{
		if (((*it)->a == obj1->body || (*it)->b == obj1->body) && ((*it)->a == obj2->body || (*it)->b == obj2->body))
		{

			cpSpaceRemoveJoint(space, *it);
			joints.erase(it);
		}
		else
			++it;
	}
}

void Physics::RemoveJoints(const PhysicsObject *obj)
{
	std::vector<cpJoint *> joints = GetJointsOfObject(obj);
	std::vector<cpJoint *>::const_iterator it = joints.begin();
	while (it != joints.end())
	{
		RemoveJoint(*it);
		++it;
	}
}

void Physics::RemoveJoint(const Joint &joint)
{
	RemoveJoint(joint.joint);
}

void Physics::RemoveJoint(const cpJoint *joint)
{
	std::vector<cpJoint *>::iterator it = std::find(joints.begin(), joints.end(), joint);
	if (it != joints.end())
	{
		cpSpaceRemoveJoint(space, *it);
		joints.erase(it);
	}
}

const std::vector<cpJoint *> Physics::GetJointsOfObject(const PhysicsObject *obj) const
{
	assert(obj->body != NULL);

	std::vector<cpJoint *> j;

	std::vector<cpJoint *>::const_iterator it = joints.begin();
	while (it != joints.end())
	{
		if ((*it)->a == obj->body || (*it)->b == obj->body)
		{
			j.push_back(*it);
		}
		++it;
	}
	return j;
}

PhysicsObject *Physics::findObjectByBody(cpBody *body) const
{
	if (!body)
	{
		return NULL;
	}
	std::map<cpBody *, PhysicsObject *>::const_iterator it = body_to_object.find(body);
	if (it != body_to_object.end())
	{
		return it->second;
	}
	return NULL;
}

// Find object with this body and this shape
PhysicsObject *Physics::FindObject(cpBody *body, cpShape *shape)
{
	PhysicsObject *obj = findObjectByBody(body);
	if (obj)
	{
		std::vector<cpShape *>::const_iterator sit = obj->shapes.begin();
		int count = 0;
		while (sit != obj->shapes.end())
		{
			if (*sit == shape)
			{
				obj->colliding_shape_index = count;
				return obj;
			}
			++count;
			++sit;
		}
	}
	return NULL;
}

// Find object with this shape
PhysicsObject *Physics::FindObject(cpShape *shape)
{
	// TODO. Use shape_to_obj map, if at all possible
	std::vector<PhysicsObject *>::const_iterator it = objects.begin();
	while (it != objects.end())
	{
		std::vector<cpShape *>::const_iterator sit = (*it)->shapes.begin();
		int count = 0;
		while (sit != (*it)->shapes.end())
		{
			if (*sit == shape)
			{
				(*it)->colliding_shape_index = count;
				return *it;
			}
			++count;
			++sit;
		}
		++it;
	}
	return NULL;
}

std::set<PhysicsObject *> Physics::GetJoinedPhysicsObjects(const PhysicsObject *obj1) const
{
	const std::vector<cpJoint *> j = GetJointsOfObject(obj1);
	std::set<PhysicsObject *> v;

	std::vector<cpJoint *>::const_iterator it = j.begin();
	while (it != j.end())
	{
		cpBody *body;
		if ((*it)->a == obj1->body)
		{
			body = (*it)->b;
		}
		else
		{
			body = (*it)->a;
		}
		PhysicsObject *obj = findObjectByBody(body);
		if (obj)
		{
			v.insert(obj);
			// ???? Can we find this body in more joints?
		}
		++it;
	}
	return v;
}

/***********************************************PhysicsObject**************************/

PhysicsObject::PhysicsObject(cpFloat mass, cpFloat inertia, Physics *physics, bool is_static)
	: physics(physics), colliding_shape_index(0), is_static(is_static)
{
	assert(physics != NULL);
	body = cpBodyNew(mass, inertia);
	if (!is_static)
	{
		assert(physics->space != NULL);
		cpSpaceAddBody(physics->space, body);
	}
	shapes.clear();
}

PhysicsObject::~PhysicsObject()
{
}

void PhysicsObject::AddCircleShape(cpFloat radius, const Vector2 &offset, cpFloat elasticity, cpFloat friction)
{
	assert(body != NULL);
	cpShape *shape = cpCircleShapeNew(body, radius, cpv(offset.x, offset.y));
	assert(shape != NULL);
	shape->e = elasticity;
	shape->u = friction;
	if (physics->space != NULL)
	{
		if (is_static)
			cpSpaceAddStaticShape(physics->space, shape);
		else
			cpSpaceAddShape(physics->space, shape);
	}
	shapes.push_back(shape);
}

void PhysicsObject::AddSegmentShape(const Vector2 &begin, const Vector2 &end, cpFloat radius, cpFloat elasticity,
									cpFloat friction)
{
	assert(body != NULL);
	cpShape *shape = cpSegmentShapeNew(body, cpv(begin.x, begin.y), cpv(end.x, end.y), radius);
	assert(shape != NULL);
	shape->e = elasticity;
	shape->u = friction;
	if (physics->space != NULL)
	{
		if (is_static)
			cpSpaceAddStaticShape(physics->space, shape);
		else
			cpSpaceAddShape(physics->space, shape);
	}
	shapes.push_back(shape);
}

void PhysicsObject::AddPolyShape(int numVerts, Vector2 *vectors, const Vector2 &offset, cpFloat elasticity,
								 cpFloat friction)
{
	assert(body != NULL);
	assert(sizeof(Vector2) == sizeof(cpVect));

	cpShape *shape = cpPolyShapeNew(body, numVerts, (cpVect *)vectors, cpv(offset.x, offset.y));
	assert(shape != NULL);
	shape->e = elasticity;
	shape->u = friction;
	if (physics->space != NULL)
	{
		if (is_static)
			cpSpaceAddStaticShape(physics->space, shape);
		else
			cpSpaceAddShape(physics->space, shape);
	}
	shapes.push_back(shape);
}

void PhysicsObject::RemoveShape(int shape_index)
{
	assert((int)shapes.size() > shape_index);

	std::vector<cpShape *>::iterator it = shapes.begin();
	int count = 0;
	while (it != shapes.end())
	{
		if (shape_index == count)
		{
			if (is_static)
			{
				cpSpaceRemoveStaticShape(physics->space, *it);
				cpSpaceRehashStatic(physics->space);
			}
			else
			{
				cpSpaceRemoveShape(physics->space, *it);
				cpSpaceHashRehash(physics->space->activeShapes);
			}
			shapes.erase(it);
			return;
		}
		count++;
		++it;
	}
}

float PhysicsObject::GetFriction(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->u;
}

float PhysicsObject::GetElasticity(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->e;
}

void PhysicsObject::SetAngularVelocity(cpFloat w)
{
	assert(body != NULL);
	body->w = w;
}

void PhysicsObject::SetVelocity(const Vector2 &v)
{
	assert(body != NULL);
	body->v = cpv(v.x, v.y);
}

void PhysicsObject::SetCollisionType(unsigned int type, int shape_index)
{
	assert((int)shapes.size() > shape_index);
	shapes[shape_index]->collision_type = type;
}

void PhysicsObject::SetGroup(unsigned int group, int shape_index)
{
	assert((int)shapes.size() > shape_index);
	shapes[shape_index]->group = group;
}

void PhysicsObject::SetLayers(unsigned int layers, int shape_index)
{
	assert((int)shapes.size() > shape_index);
	shapes[shape_index]->layers = layers;
}

void PhysicsObject::SetData(void *data, int shape_index)
{
	assert((int)shapes.size() > shape_index);
	shapes[shape_index]->data = data;
}

unsigned int PhysicsObject::GetCollisionType(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->collision_type;
}

unsigned int PhysicsObject::GetGroup(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->group;
}

unsigned int PhysicsObject::GetLayers(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->layers;
}

void *PhysicsObject::GetData(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->data;
}

int PhysicsObject::GetShapeType(int shape_index) const
{
	assert((int)shapes.size() > shape_index);
	return shapes[shape_index]->type;
}

int PhysicsObject::GetNumberVertices(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)POLY_SHAPE);
	return ((cpPolyShape *)shapes[shape_index])->numVerts;
}

Vector2 PhysicsObject::GetVertex(int vertex_index, int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)POLY_SHAPE);
	cpVect position = cpvadd(body->p, cpvrotate(((cpPolyShape *)shapes[shape_index])->verts[vertex_index], body->rot));
	return Vector2(position.x, position.y);
}

Vector2 PhysicsObject::GetSegmentShapeBegin(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)SEGMENT_SHAPE);
	cpVect position = cpvadd(body->p, cpvrotate(((cpSegmentShape *)shapes[shape_index])->a, body->rot));
	return Vector2(position.x, position.y);
}

Vector2 PhysicsObject::GetSegmentShapeEnd(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)SEGMENT_SHAPE);
	cpVect position = cpvadd(body->p, cpvrotate(((cpSegmentShape *)shapes[shape_index])->b, body->rot));
	return Vector2(position.x, position.y);
}

float PhysicsObject::GetSegmentShapeRadius(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)SEGMENT_SHAPE);
	return (float)((cpSegmentShape *)shapes[shape_index])->r;
}

float PhysicsObject::GetCircleShapeRadius(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)CIRCLE_SHAPE);
	return (float)((cpCircleShape *)shapes[shape_index])->r;
}

Vector2 PhysicsObject::GetCircleShapeCenter(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)CIRCLE_SHAPE);
	cpVect position = cpvadd(body->p, cpvrotate(((cpCircleShape *)shapes[shape_index])->c, body->rot));
	return Vector2(position.x, position.y);
}

Vector2 PhysicsObject::GetCircleShapeOffset(int shape_index) const
{
	assert((int)shapes.size() > shape_index && shapes[shape_index]->type == (cpShapeType)CIRCLE_SHAPE);
	cpVect position = ((cpCircleShape *)shapes[shape_index])->c;
	return Vector2(position.x, position.y);
}

void PhysicsObject::UpdateVelocity()
{
	assert(is_static && physics->space != NULL);
	cpBodyUpdateVelocity(body, physics->space->gravity, physics->space->damping, physics->delta);
}

void PhysicsObject::UpdatePosition()
{
	assert(is_static && physics->space != NULL);
	cpBodyUpdatePosition(body, physics->delta);
	cpSpaceRehashStatic(physics->space);
}

int PhysicsObject::GetNumberOfShapes() const
{
	return shapes.size();
}

int PhysicsObject::GetCollidingShapeIndex() const
{
	return colliding_shape_index;
}

std::pair<Vector2, Vector2> Joint::GetPoints() const
{
	std::pair<Vector2, Vector2> v;

	cpVect pos1 = joint->a->p;
	cpVect pos2 = joint->b->p;
	cpVect s, e, v1, v2;
	switch (joint->type)
	{
	case CP_PIN_JOINT:
		v1 = ((cpPinJoint *)(joint))->anchr1;
		s = cpvadd(pos1, cpvrotate((joint)->a->rot, v1));
		v2 = ((cpPinJoint *)(joint))->anchr2;
		e = cpvadd(pos2, cpvrotate((joint)->b->rot, v2));
		break;
	case CP_SLIDE_JOINT:
		v1 = ((cpSlideJoint *)(joint))->anchr1;
		s = cpvadd(pos1, cpvrotate((joint)->a->rot, v1));
		v2 = ((cpSlideJoint *)(joint))->anchr2;
		e = cpvadd(pos2, cpvrotate((joint)->b->rot, v2));
		break;
	case CP_PIVOT_JOINT:
		v1 = ((cpPivotJoint *)(joint))->anchr1;
		s = pos1; // cpvadd(pos1, cpvrotate(v1, (joint)->a->rot));
		v2 = ((cpPivotJoint *)(joint))->anchr2;
		e = pos2; // cpvadd(pos2, cpvrotate(v2, (joint)->b->rot));
		break;
	case CP_GROOVE_JOINT:
		// TODO. What to do with these?
		assert(0);
		break;
	case CP_NUM_JOINTS:
	default:
		// This should never happen.
		assert(0);
		break;
	}

	v.first = Vector2(s.x, s.y);
	v.second = Vector2(e.x, e.y);
	return v;
}