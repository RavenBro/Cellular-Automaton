#include "ECSClass.h"
#include <algorithm>

Entity::Entity()
{
    if (Entity::reusabaleIds.size() == 0)
    {
        id = Entity::currentId++;
    }
    else
    {
        id = Entity::reusabaleIds[Entity::reusabaleIds.size() - 1];
        Entity::reusabaleIds.pop_back();
    }
    Entity::entities.push_back(this);
}

Entity::~Entity()
{
    if (id == Entity::currentId - 1)
    {
        Entity::currentId--;
    }
    else
    {
        Entity::reusabaleIds.push_back(id);
    }
    for (size_t i = 0; i < components.size(); i++)
    {
        delete components[i];
    }
    Entity::entities.erase(std::find(Entity::entities.begin(),Entity::entities.end(), this));
}

bool Entity::HasComponent(unsigned id, unsigned &componentPos)
{
    for(size_t i = 0; i < components.size(); i++)
    {
        if (components[i]->type == id){
            componentPos = i;
            return true;
        }
    }
    return false;
}

bool Entity::HasComponent(unsigned n, unsigned* ids, unsigned* componentPos)
{
    unsigned counter = 0;
    for (size_t j = 0; j < n; j++)
    {
        unsigned id = ids[j];
        for(size_t i = 0; i < components.size(); i++)
        {
            if (components[i]->type == id){
                componentPos[j] = i;
                counter++;
                break;
            }
        }
    }
    if (counter == n)
        return true;
    else
        return false;
}

bool Entity::isHaveTag(std::string tag)
{
    unsigned id;
    if (HasComponent(TagComponent::GetCompId(), id))
    {
        auto tags = static_cast<TagComponent*>(components[id])->tags;
        for (size_t i = 0; i < tags.size(); i++)
        {
            if (tags[i] == tag)
                return true;
        }
    }
    return false;
}

void Entity::giveTag(std::string tag) 
{
    unsigned id;
    if (HasComponent(TagComponent::GetCompId(), id))
    {
        auto tags = static_cast<TagComponent*>(components[id])->tags;
        tags.push_back(tag);
    }
    else
    {
        TagComponent* t = new TagComponent();
        t->tags.push_back(tag);
        components.push_back(t);
    }
}


// TestComponent1::TestComponent1(int i)
// {
//     type = 0;
//     x = i;
// }

// TestComponent2::TestComponent2(std::string str)
// {
//     type = 1;
//     s = str;
// }

// Entity* CreateEnt1(int x)
// {
//     Entity* e = new Entity();
//     e->components.push_back(new TestComponent1(x));
//     return e;
// }

// Entity* CreateEnt2(std::string s)
// {
//     Entity* e = new Entity();
//     e->components.push_back(new TestComponent2(s));
//     return e;
// }

// Entity* CreateEnt3(int x, std::string s)
// {
//     Entity* e = new Entity();
//     e->components.push_back(new TestComponent1(x));
//     e->components.push_back(new TestComponent2(s));
//     return e;
// }