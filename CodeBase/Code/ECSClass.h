#include <GL/glew.h>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include "SOIL.h"
#include "./shaderClass.h"

enum Element {empty, conductor, tail, head};

class Component
{
    public:
        unsigned int type; 
};

class SpriteComponent : public Component
{
    public:
        float pos[2];
        float size[2];
        GLuint VBO, VAO, EBO;
        Shader spriteShader;
        GLuint spriteTexture;
        static unsigned int typeId;
        
        SpriteComponent(float,float,float,float,std::string);
        SpriteComponent(float,float,float,float,std::string,std::string,std::string);
        ~SpriteComponent();
        static unsigned GetCompId() {return typeId;}
};

class InputComponent : public Component
{
    public:
        double mousePos[2];
        static unsigned int typeId;
        static unsigned GetCompId() {return typeId;}
        InputComponent();
};

class StateDescriptionComponent : public Component
{
    public:
        double worldShift[2];
        std::chrono::_V2::system_clock::time_point old_time, cur_time;
        float timeDelta;
        float block_size;
        float block_size_borders[2];
        Element chosedElement;
        bool isGamePaused;
        float gameSpeed;
        float baseSpeed;

        static unsigned int typeId;
        static unsigned GetCompId() {return typeId;}
        StateDescriptionComponent();
};

class TagComponent : public Component
{
    public:
        std::vector<std::string> tags;
        static unsigned int typeId;
        static unsigned GetCompId() {return typeId;}
        TagComponent();
};

class SchemeElement
{
    public:
        Element type;
        int pos[2]; 
        SchemeElement(Element t, int x, int y) : type{t}, pos{x,y} {} 
};

class SchemeInfoComponent : public Component
{
    public:
        std::vector<SchemeElement*> elements;
        static unsigned int typeId;
        static unsigned GetCompId() {return typeId;}
        SchemeInfoComponent();
        void AddElement(Element t, int x, int y) {elements.push_back(new SchemeElement(t,x,y));}
};

class ButtonComponent : public Component
{
    public:
        float pos[2], size[2];
        int layer;
        static unsigned int typeId;
        static unsigned GetCompId() {return typeId;}
        std::function<void(float, float, int)> func;
        ButtonComponent(int, float, float, float, float, std::function<void(float, float, int)>);
};

class Entity
{
    public:
        unsigned int id;
        std::vector<Component*> components;
        static std::vector<int> reusabaleIds;
        static std::vector<Entity*> entities;
        static unsigned int currentId;
        Entity();
        bool HasComponent(unsigned, unsigned&);
        bool HasComponent(unsigned, unsigned*, unsigned*);
        void giveTag(std::string);
        bool isHaveTag(std::string);

        static StateDescriptionComponent* GetStateDescriptor();
        static SchemeInfoComponent* GetSchemeInfo();
    private:
        ~Entity();
};

Entity* createCellTemplate(float, float, float, float, std::string);
Entity* createCellTemplate(float, float, float, float, std::string, std::string, std::string);
Entity* createStateDescriptionTemplate();
Entity* createSchemeTemplate();