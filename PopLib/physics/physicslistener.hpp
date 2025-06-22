/* Sexy Chipmunk, a physics engine for the PopCap Games Framework using Scott Lembcke's excellent chipmunk physics
 * library */
/* Copyright (c) 2007 W.P. van Paassen
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

#ifndef __PHYSICSLISTENER_HPP__
#define __PHYSICSLISTENER_HPP__
#ifdef _WIN32
#pragma once
#endif

namespace PopLib
{

class PhysicsObject;
class CollisionObject;
class Graphics;

class PhysicsListener
{
  public:
	virtual ~PhysicsListener()
	{
	}

	virtual void DrawPhysicsObject(PhysicsObject *object, Graphics *g)
	{
	}
	virtual void HandleCollision(CollisionObject *col)
	{
	}
	virtual bool HandleTypedCollision(CollisionObject *col)
	{
		return true;
	}
	virtual void BeforePhysicsStep()
	{
	}
	virtual void AfterPhysicsStep()
	{
	}
};

}; // namespace PopLib

#endif
