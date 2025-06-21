#ifndef __SMARTPTR_HPP__
#define __SMARTPTR_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"

#pragma pack(push, 8) // need 8 byte alignment for InterlockedIncrement

namespace PopLib
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RefCount
{
  private:
	mutable long mRefCount;

  protected:
	// The compiler allows a derived destructor to be called if it's not explicitly declared
	// even if the parent destructor is private, so I make the parent destructor protected so that
	// derived classes can make their destructors protected as well.  (They get an error if the
	// parent destructor is private.)
	virtual ~RefCount()
	{
	}

  public:
	RefCount() : mRefCount(0)
	{
	}

	const RefCount *CreateRef() const
	{
#ifdef WIN32
		InterlockedIncrement(&mRefCount);
#else
		mRefCount++;
#endif
		return this;
	}

	void Release()
	{
#ifdef WIN32
		if (InterlockedDecrement(&mRefCount) <= 0)
			delete this;
#else
		if (--mRefCount <= 0)
			delete this;
#endif
	}

	// You might want to assign one reference counted object to another simply to copy
	// the member variables of one to the other, but you certainly don't want to copy
	// the reference counts!
	RefCount(const RefCount &) : mRefCount(0)
	{
	}
	RefCount &operator=(const RefCount &)
	{
		return *this;
	}

	ulong GetRefCount()
	{
		return mRefCount;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Const smart pointer.  Automatically performs reference counting on objects which
// implement the CreateRef and Release interface.  Allows const access to underlying object.
template <class T> class ConstSmartPtr
{
  protected:
	T *p;

  public:
	ConstSmartPtr() : p(NULL)
	{
	}

	// Copy constructor.  Assign and add reference to underlying object.
	ConstSmartPtr(const T *theObject) : p((T *)(theObject ? theObject->CreateRef() : NULL))
	{
	}
	ConstSmartPtr(const ConstSmartPtr &theCopy) : p((T *)(theCopy ? theCopy.p->CreateRef() : NULL))
	{
	}

	// Destructor.  Remove reference to underlying object.
	~ConstSmartPtr()
	{
		if (p != NULL)
			p->Release();
	}

	// Arrow operator allows ConstSmartPtr to be treated like actual pointer.
	const T *operator->() const
	{
		return p;
	}

	// Type case operator allows ConstSmartPtr to be cast to const T*,
	operator const T *() const
	{
		return p;
	}

	// Assignment operator.  Release old underlying object if not null.  Add reference to new object.
	const T *operator=(const T *thePtr)
	{
		if (p != thePtr) // prevent self-assignment
		{
			if (p != NULL)
				p->Release();
			p = (T *)(thePtr ? thePtr->CreateRef() : NULL);
		}

		return thePtr;
	}

	const ConstSmartPtr &operator=(const ConstSmartPtr &theCopy)
	{
		operator=(theCopy.get());
		return *this;
	}

	// Allow comparions just like normal pointer.
	bool operator==(const T *thePtr) const
	{
		return p == thePtr;
	}
	bool operator!=(const T *thePtr) const
	{
		return p != thePtr;
	}
	bool operator<(const T *thePtr) const
	{
		return p < thePtr;
	}

	// Accessor to actual object
	const T *get() const
	{
		return p;
	}

	struct Comp
	{
		bool operator()(const ConstSmartPtr &a, const ConstSmartPtr &b) const
		{
			return a.get() < b.get();
		}
	};
};

// Smart pointer.  Automatically performs reference counting on objects which
// implement the CreateRef and Release interface.  Allows non-const access to underlying object.
template <class T> class SmartPtr : public ConstSmartPtr<T>
{
  public:
	SmartPtr()
	{
	}
	SmartPtr(T *theObject) : ConstSmartPtr<T>(theObject)
	{
	}
	SmartPtr(const SmartPtr &theCopy) : ConstSmartPtr<T>(theCopy.p)
	{
	}

	const SmartPtr &operator=(const SmartPtr &theCopy)
	{
		ConstSmartPtr<T>::operator=(theCopy.get());
		return *this;
	}

	T *operator=(T *thePtr)
	{
		ConstSmartPtr<T>::operator=(thePtr);
		return thePtr;
	}

	T *operator->() const
	{
		return this->p;
	}
	operator T *() const
	{
		return this->p;
	}

	T *get() const
	{
		return this->p;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef SmartPtr<RefCount> RefCountPtr;

} // namespace PopLib

#pragma pack(pop)

// Helper macro to forward declare reference counted classes along with their smart pointer definitions
#define PTR_FORWARD(X)                                                                                         \
	class X;                                                                                                           \
	typedef SmartPtr<X> X##Ptr
#define PTR_FORWARDNS(X)                                                                                       \
	class X;                                                                                                           \
	typedef PopLib::SmartPtr<X> X##Ptr

#endif