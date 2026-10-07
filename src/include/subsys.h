// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "pengine.h"
#include "vmath.h"
#include <string>
#include <vector>

class PApp;

///
/// @brief A PResource basically is just a name
///
class PResource
{
protected:
	std::string name;

public:
	const std::string & getName() const
	{
		return name;
	}
};

///
/// @brief Implementation of a buffer where you can push inside stuff and clear everything if you want
/// @todo Actually is not a list. Why is called this way?
///
template <class T>
class PResourceList
{
private:

	std::vector<T *> reslist;

public:

	~PResourceList()
	{
		clear();
	}

	T * add(T *newresource)
	{
		reslist.push_back(newresource);
		return newresource;
	}

	T * find(const std::string &name)
	{
		for (T *res: reslist)
			if (name == res->getName())
				return res;
		return nullptr;
	}

	void clear()
	{
		for (T *res: reslist)
			delete res;

		reslist.clear();
	}
};

class PSubsystem
{
protected:

    PApp &app;

public:

    PSubsystem(PApp &parentApp):
        app(parentApp)
    {
    }

    virtual ~PSubsystem()
    {
    }

    virtual void tick(float delta, const vec3f &eyepos, const mat44f &eyeori, const vec3f &eyevel)
    {
        UNREFERENCED_PARAMETER(delta);
        UNREFERENCED_PARAMETER(eyepos);
        UNREFERENCED_PARAMETER(eyeori);
        UNREFERENCED_PARAMETER(eyevel);
    }
};

