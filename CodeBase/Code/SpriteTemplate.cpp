#include "ECSClass.h"
#include <iostream>

unsigned int SpriteComponent::typeId = 1;
unsigned int InputComponent::typeId = 2;
unsigned int StateDescriptionComponent::typeId = 3;
unsigned int TagComponent::typeId = 4;
unsigned int SchemeInfoComponent::typeId = 5;
unsigned int ButtonComponent::typeId = 6;


SpriteComponent::SpriteComponent(float x, float y, float w, float h, std::string path)
    :SpriteComponent(x, y, w, h, path, "./Code/Shaders/spriteVertex.vs", "./Code/Shaders/spriteFragment.fs") {}

SpriteComponent::SpriteComponent(float x, float y, float w, float h, std::string path, const std::string vsPath, const std::string fsPath)
    :spriteShader(vsPath.c_str(), fsPath.c_str())
{
    pos[0] = x;
    pos[1] = y;
    size[0] = w;
    size[1] = h;
    type = typeId;

    int imageW, imageH;
    unsigned char* image = SOIL_load_image(path.c_str(), &imageW, &imageH, 0, SOIL_LOAD_RGBA);


    glGenTextures(1, &spriteTexture);
    glBindTexture(GL_TEXTURE_2D, spriteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageW, imageH, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLfloat vertices[] = {
     1.0f,  1.0f, 0.0f,   1.0f, 1.0f,   
     1.0f,  0.0f, 0.0f,   1.0f, 0.0f,   
     0.0f,  0.0f, 0.0f,   0.0f, 0.0f,   
     0.0f,  1.0f, 0.0f,   0.0f, 1.0f    
    };
    GLuint indices[] = {
        0, 1, 3,
        1, 2, 3  
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO
}

SpriteComponent::~SpriteComponent()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

InputComponent::InputComponent() : mousePos{0, 0}
{
    type = typeId;
};

StateDescriptionComponent::StateDescriptionComponent() : worldShift{0, 0}, block_size{50.0}, block_size_borders{24,101}
{
    chosedElement = conductor;
    gameSpeed = 0.3;
    baseSpeed = 0.5f;
    type = typeId;
    isGamePaused = true;
    timeDelta = 0;
};

TagComponent::TagComponent()
{
    type = typeId;
};

Entity* createStateDescriptionTemplate()
{
    auto inp = new Entity();
    inp->components.push_back(new StateDescriptionComponent());
    inp->components.push_back(new InputComponent());
    return inp;
}

SchemeInfoComponent::SchemeInfoComponent()
{
    type = typeId;
};

ButtonComponent::ButtonComponent(int l, float x, float y, float w, float h, std::function<void(float, float, int)> f) : pos{x,y}, size{w,h}
{
    type = typeId;
    func = f;
    layer = l;
}


Entity* createSchemeTemplate()
{
    auto inp = new Entity();
    inp->components.push_back(new SchemeInfoComponent());
    return inp;
}

Entity* createCellTemplate(float x, float y, float width, float height, std::string imagePath)
{
    auto cell = new Entity();
    cell->components.push_back(new SpriteComponent(x,y,width,height,imagePath));
    return cell;
}

Entity* createCellTemplate(float x, float y, float width, float height, std::string imagePath, std::string VSpath, std::string FSpath)
{
    auto cell = new Entity();
    cell->components.push_back(new SpriteComponent(x,y,width,height,imagePath,VSpath,FSpath));
    return cell;
}

StateDescriptionComponent* Entity::GetStateDescriptor()
{
    StateDescriptionComponent* state = nullptr;
    unsigned id;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        if(Entity::entities[i]->HasComponent(StateDescriptionComponent::GetCompId(),id))
        {
            state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[id]);
            break;
        }
    }
    return state;
}   

SchemeInfoComponent* Entity::GetSchemeInfo()
{
    SchemeInfoComponent* scheme = nullptr;
    unsigned id;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        if(Entity::entities[i]->HasComponent(SchemeInfoComponent::GetCompId(),id))
        {
            scheme = static_cast<SchemeInfoComponent*>(Entity::entities[i]->components[id]);
            break;
        }
    }
    return scheme;
}   