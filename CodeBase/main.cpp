#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS 1
#include "./glm/glm/glm.hpp"
#include "./glm/glm/gtc/matrix_transform.hpp"
#include "./glm/glm/gtc/type_ptr.hpp"
#include <cmath>
#include <chrono>
#include <csignal>
#include <algorithm>
#include <fstream>

#include "./Code/EventClasses.h"
#include "./Code/shaderClass.h"
#include "./Code/ECSClass.h"
#include "SOIL.h"

using namespace glm;


const GLuint WIDTH = 800, HEIGHT = 600;

GLFWwindow* MyInit(GLuint,GLuint);
void initializeGame();
void saveSchemeToFile();
void loadSchemeFromFile();
void createMenuUV();
GLFWwindow* globalWindow = MyInit(WIDTH, HEIGHT);

EventHandler* EventHandler::handler = nullptr;
unsigned int Entity::currentId = 0;
std::vector<Entity*> Entity::entities = std::vector<Entity*>{};
std::vector<int> Entity::reusabaleIds = std::vector<int>{};

void MyBufferClearSystem(GLfloat, GLfloat, GLfloat);
void SpriteWithTagRenderSystem(std::string);
void BackgroundRenderSystem();
void WindowCloseEventHandler(GLFWwindow*);
void GamePauseHandler(GLFWwindow*);
void WindowButtonHandler(GLFWwindow*);
void UpdateInputSystem();
void MoveWorldSystem();
void DrawSchemeSystem();
void SteamPunkSystem();
void clearEventsSystem();
void UpdateWorldTimeSystem();

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode); //My handmade key enets handler
void mouse_callback(GLFWwindow* window, int, int, int);

float modulus(float x, float y){return sign(x) * (abs(x) - y * (int)(abs(x) / y));}

int main()
{
    glfwSetKeyCallback(globalWindow, key_callback); // Set the required callback functions
    glfwSetMouseButtonCallback(globalWindow, mouse_callback);

    initializeGame();

    // Game loop
    while (!glfwWindowShouldClose(globalWindow))
    {
        // Event handling
        glfwPollEvents();
        WindowCloseEventHandler(globalWindow);
        GamePauseHandler(globalWindow);

        WindowButtonHandler(globalWindow);
        clearEventsSystem();

        // Systems
        MyBufferClearSystem(1, 1, 1);
        
        UpdateWorldTimeSystem();
        UpdateInputSystem();
        MoveWorldSystem();
        SteamPunkSystem();

        BackgroundRenderSystem();
        SpriteWithTagRenderSystem("simpleSprite");
        DrawSchemeSystem();
        SpriteWithTagRenderSystem("panelSprite");
        SpriteWithTagRenderSystem("menuSprite");
        SpriteWithTagRenderSystem("helpPopup");

        // Swap the screen buffers
        glfwSwapBuffers(globalWindow);
    }
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    EventHandler::GetHandler()->NewEvent("keyboardevent", key, action);
} 

void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(globalWindow, &xpos, &ypos);
    EventHandler::GetHandler()->NewEvent("mouseevent", button, action, xpos, HEIGHT - ypos);
} 

void WindowCloseEventHandler(GLFWwindow *window)
{
    std::function<void(Event*)> f = [&](Event *p)
    {
        saveSchemeToFile();

        if (static_cast<KeyBoardEvent*>(p)->key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GL_TRUE);
    };
    EventHandler::GetHandler()->ApplyFunctionToEvents("keyboardevent", f);
};

void GamePauseHandler(GLFWwindow *window)
{
    std::function<void(Event*)> f = [&](Event *p)
    {
        auto keyEvent = static_cast<KeyBoardEvent*>(p);
        if (keyEvent->key == GLFW_KEY_SPACE && keyEvent->action == GLFW_PRESS)
        {
            StateDescriptionComponent *state = nullptr;
            for (size_t i = 0; i < Entity::entities.size(); i++)
            {
                unsigned stateDescrId;
                if (Entity::entities[i]->HasComponent(StateDescriptionComponent::GetCompId(), stateDescrId))
                {
                    state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[stateDescrId]);
                    break;
                }
            }
            state->isGamePaused = !state->isGamePaused;
        }
    };
    EventHandler::GetHandler()->ApplyFunctionToEvents("keyboardevent", f);
};


void WindowButtonHandler(GLFWwindow *window)
{
    std::function<void(Event*)> f = [&](Event *p)
    {
        MouseEvent* event = static_cast<MouseEvent*>(p);
        if (event->action == GLFW_PRESS)
        {
            
            std::vector<ButtonComponent*> buts;
            for (size_t i = 0; i < Entity::entities.size(); i++)
            {
                unsigned id;
                if (Entity::entities[i]->HasComponent(ButtonComponent::GetCompId(),id))
                {
                    auto but = static_cast<ButtonComponent*>(Entity::entities[i]->components[id]);
                    if (event->pos[0] < but->pos[0] || event->pos[0] > but->pos[0] + but->size[0] ||
                        event->pos[1] < but->pos[1] || event->pos[1] > but->pos[1] + but->size[1])
                        continue;
                    buts.push_back(but);
                }
            }
            std::sort(buts.begin(), buts.end(),
                [] (ButtonComponent* const& a, ButtonComponent* const& b) { return a->layer > b->layer; });
            
            buts[0]->func(event->pos[0], event->pos[1], event->button);
        }
    };
    EventHandler::GetHandler()->ApplyFunctionToEvents("mouseevent", f);
}

void clearEventsSystem()
{
    EventHandler::GetHandler()->clearEvents();
}

void MyBufferClearSystem(GLfloat r, GLfloat g, GLfloat b)
{
    glClearColor(r,g,b,1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SpriteWithTagRenderSystem(std::string tag)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLint TransformUniformPos;
    bool firstSprite = true;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned id;
        if (Entity::entities[i]->HasComponent(SpriteComponent::GetCompId(), id))
        {
            if (!Entity::entities[i]->isHaveTag(tag))
                continue;

            SpriteComponent *sprite = static_cast<SpriteComponent*>(Entity::entities[i]->components[id]);
            
            if (firstSprite)
            {
                TransformUniformPos = glGetUniformLocation(sprite->spriteShader.Program, "Transform");
                firstSprite = false;
            }
            sprite->spriteShader.Use();
            glUniform4f(TransformUniformPos, sprite->size[0] / WIDTH, sprite->size[1] / HEIGHT, sprite->pos[0] / WIDTH, sprite->pos[1] / HEIGHT);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite->EBO);
            glBindVertexArray(sprite->VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sprite->spriteTexture);
            glUniform1i(glGetUniformLocation(sprite->spriteShader.Program, "Texture"), 0);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
    glDisable(GL_BLEND);
}

void BackgroundRenderSystem()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    StateDescriptionComponent *state = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned stateDescrId;
        if (Entity::entities[i]->HasComponent(StateDescriptionComponent::GetCompId(), stateDescrId))
        {
            state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[stateDescrId]);
            break;
        }
        if (i == Entity::entities.size())
            std::cout << "ERROR!" << std::endl;
    }
    const float BLOCK_SIZE = state->block_size;
    float realShift[] = {modulus((float)state->worldShift[0],BLOCK_SIZE/WIDTH), modulus((float)state->worldShift[1],BLOCK_SIZE/HEIGHT)};

    unsigned id;
    Entity* pattern;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        if(Entity::entities[i]->isHaveTag("BackgroundSprite"))
        {
            pattern = Entity::entities[i];
            break;
        }
    }
    
    pattern->HasComponent(SpriteComponent::GetCompId(), id);
    SpriteComponent *sprite = static_cast<SpriteComponent*>(pattern->components[id]);
    GLint TransformUniformPos = glGetUniformLocation(sprite->spriteShader.Program, "Transform");
    GLfloat transforms[7056];
    
    int backNumbers[] = {int(ceil((float)WIDTH / BLOCK_SIZE)) + 2, int(ceil((float)HEIGHT / BLOCK_SIZE)) + 2};
    for (int i = 0; i < backNumbers[0]; i++)
    {
        for (int j = 0; j < backNumbers[1]; j++)
        {
            transforms[(i * backNumbers[1] + j) * 4 + 0] = BLOCK_SIZE / WIDTH;
            transforms[(i * backNumbers[1] + j) * 4 + 1] = BLOCK_SIZE / HEIGHT;
            transforms[(i * backNumbers[1] + j) * 4 + 2] = 1.0 / WIDTH * (i - 1) * BLOCK_SIZE - realShift[0];
            transforms[(i * backNumbers[1] + j) * 4 + 3] = 1.0 / HEIGHT * (j - 1) * BLOCK_SIZE - realShift[1];
        }
    }
    
    sprite->spriteShader.Use();
    glUniform4fv(TransformUniformPos, backNumbers[0] * backNumbers[1], transforms);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite->EBO);
    glBindVertexArray(sprite->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite->spriteTexture);
    glUniform1i(glGetUniformLocation(sprite->spriteShader.Program, "Texture"), 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, backNumbers[0] * backNumbers[1]);
    glBindVertexArray(0);
}

void UpdateInputSystem()
{
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned id;
        if (Entity::entities[i]->HasComponent(InputComponent::GetCompId(), id))
        {
            double xpos, ypos;
            glfwGetCursorPos(globalWindow, &xpos, &ypos);
            auto input = static_cast<InputComponent*>(Entity::entities[i]->components[id]);
            input->mousePos[0] = xpos / WIDTH;
            input->mousePos[1] = 1 - ypos / HEIGHT;
        }
    }
}

void UpdateWorldTimeSystem()
{
    static float timeCounter = 0;
    StateDescriptionComponent *state = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned id;
        if (Entity::entities[i]->HasComponent(StateDescriptionComponent::GetCompId(), id))
        {
            state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[id]);
        }
    }
    if (state->old_time == state->cur_time)
    {
        state->old_time = std::chrono::high_resolution_clock::now();
        return;
    } 
    state->cur_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::milli> delta = state->cur_time - state->old_time;
    state->timeDelta = (float)delta.count() * 0.001;
    state->old_time = state->cur_time;
}

void MoveWorldSystem()
{
    StateDescriptionComponent *state = nullptr;
    InputComponent *input = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned ids[2];
        unsigned types[] = {StateDescriptionComponent::GetCompId(), InputComponent::GetCompId()};
        if (Entity::entities[i]->HasComponent(2, types, ids))
        {
            state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[ids[0]]);
            input =  static_cast<InputComponent*>(Entity::entities[i]->components[ids[1]]);
            break;
        }
        if (i == Entity::entities.size())
            std::cout << "ERROR!" << std::endl;
    }
    
    float time_delta = state->timeDelta;

    // std::cout << "FPS:   " << 1 / time_delta << std::endl;

    const float MOVE_BORDER[2] = {0.01f, 0.01f};
    float BASE_SPEED = state->baseSpeed;
    const float COEFFICIENT = (float)WIDTH / HEIGHT; 
    const float MOVE_SPEED[2] = {BASE_SPEED, COEFFICIENT * BASE_SPEED};
    if (input->mousePos[0] <= MOVE_BORDER[0])
    {
        state->worldShift[0] -= MOVE_SPEED[0] * time_delta;
    } 
    if (input->mousePos[0] >= 1 - MOVE_BORDER[0])
    {
        state->worldShift[0] += MOVE_SPEED[0] * time_delta;
    }
    if (input->mousePos[1] <= MOVE_BORDER[1])
    {
        state->worldShift[1] -= MOVE_SPEED[1] * time_delta;
    } 
    if (input->mousePos[1] >= 1 - MOVE_BORDER[1])
    {
        state->worldShift[1] += MOVE_SPEED[1] * time_delta;
    }
}

void SteamPunkSystem()
{
    StateDescriptionComponent *state = nullptr;
    SchemeInfoComponent *scheme = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned id;
        if (Entity::entities[i]->HasComponent(StateDescriptionComponent::GetCompId(), id))
        {
            state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[id]);
        }
        if (Entity::entities[i]->HasComponent(SchemeInfoComponent::GetCompId(), id))
        {
            scheme = static_cast<SchemeInfoComponent*>(Entity::entities[i]->components[id]);
        }
    }

    if (state->isGamePaused)
    {
        return;
    }

    float timeDelta = state->timeDelta;
    static float timeCounter = 0;
    timeCounter += timeDelta;
    
    while (timeCounter >= state->gameSpeed)
    {
        timeCounter -= state->gameSpeed;

        //FIRST STEP - MOVING HEADS
        std::vector<int> newComers{};
        for (size_t i = 0; i < scheme->elements.size(); i++)
        {
            if (scheme->elements[i]->type == head)
            {
                for (size_t j = 0; j < scheme->elements.size(); j++)
                {
                    if (scheme->elements[j]->type == conductor &&
                        abs(scheme->elements[i]->pos[0] - scheme->elements[j]->pos[0]) < 2 &&
                        abs(scheme->elements[i]->pos[1] - scheme->elements[j]->pos[1]) < 2)
                    {
                        newComers.push_back(j);
                    }
                }
                
            }
        }
        std::sort(newComers.begin(), newComers.end(),
            [] (int const& a, int const& b) { return a < b; });
        for (int i = 0; i < newComers.size(); i++)
        {
            bool isGood = true;
            if ((int)newComers.size() - i >= 3)
            {
                if (newComers[i] == newComers[i+2])
                {
                    isGood = false;
                }
            }
            if (isGood)
            {
                scheme->elements[newComers[i]]->type = head;
            }
            else
            {
                int curElement = newComers[i];
                while (i < newComers.size())
                {
                    if (newComers[i] == curElement)
                    {
                        i++;
                    }
                    else
                    {
                        break;
                    }
                    
                }
                i--;
            }
        }
        
        //SECOND STEP - MOVING TAILS
        newComers.resize(0);
        for (size_t i = 0; i < scheme->elements.size(); i++)
        {
            if (scheme->elements[i]->type == tail)
            {
                for (size_t j = 0; j < scheme->elements.size(); j++)
                {
                    if (scheme->elements[j]->type == head &&
                        abs(scheme->elements[i]->pos[0] - scheme->elements[j]->pos[0]) < 2 &&
                        abs(scheme->elements[i]->pos[1] - scheme->elements[j]->pos[1]) < 2)
                    {
                        newComers.push_back(j);
                    }
                }
                
            }
        }
        for (int i = 0; i < newComers.size(); i++)
        {
            scheme->elements[newComers[i]]->type = tail;
        }

        //THIRD STEP - CLEARING TAILS
        newComers.resize(0);
        for (size_t i = 0; i < scheme->elements.size(); i++)
        {
            if (scheme->elements[i]->type == tail)
            {
                newComers.push_back(i);
                for (size_t j = 0; j < scheme->elements.size(); j++)
                {
                    if (scheme->elements[j]->type == head &&
                        abs(scheme->elements[i]->pos[0] - scheme->elements[j]->pos[0]) < 2 &&
                        abs(scheme->elements[i]->pos[1] - scheme->elements[j]->pos[1]) < 2)
                    {
                        newComers.pop_back();
                        break;
                    }
                }
                
            }
        }
        for (int i = 0; i < newComers.size(); i++)
        {
            scheme->elements[newComers[i]]->type = conductor;
        }
    }
}

void DrawSchemeSystem()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    StateDescriptionComponent *state = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned stateDescrId;
        if (Entity::entities[i]->HasComponent(StateDescriptionComponent::GetCompId(), stateDescrId))
        {
            state = static_cast<StateDescriptionComponent*>(Entity::entities[i]->components[stateDescrId]);
            break;
        }
        if (i == Entity::entities.size())
            std::cout << "ERROR!" << std::endl;
    }
    const float BLOCK_SIZE = state->block_size;
    float shift[] = {(float)state->worldShift[0], (float)state->worldShift[1]};

    unsigned id;
    SpriteComponent* patterns[3];
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        if(Entity::entities[i]->isHaveTag("ConductorSprite"))
        {
            Entity::entities[i]->HasComponent(SpriteComponent::GetCompId(), id);
            patterns[0] = static_cast<SpriteComponent*>(Entity::entities[i]->components[id]);
            continue;
        }
        if(Entity::entities[i]->isHaveTag("TailSprite"))
        {
            Entity::entities[i]->HasComponent(SpriteComponent::GetCompId(), id);
            patterns[1] = static_cast<SpriteComponent*>(Entity::entities[i]->components[id]);
            continue;
        }
        if(Entity::entities[i]->isHaveTag("HeadSprite"))
        {
            Entity::entities[i]->HasComponent(SpriteComponent::GetCompId(), id);
            patterns[2] = static_cast<SpriteComponent*>(Entity::entities[i]->components[id]);
            continue;
        }
    }

    GLint TransformUniformPos = glGetUniformLocation(patterns[0]->spriteShader.Program, "Transform");
    GLint TypesUniformPos = glGetUniformLocation(patterns[0]->spriteShader.Program, "Types");
    GLfloat transforms[7056];
    GLint types[1764];

    unsigned elementsCounter = 0;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        if (Entity::entities[i]->HasComponent(SchemeInfoComponent::GetCompId(), id))
        {
            SchemeInfoComponent* scheme = static_cast<SchemeInfoComponent*>(Entity::entities[i]->components[id]);
            for (size_t j = 0; j < scheme->elements.size(); j++)
            {
                SchemeElement* element = scheme->elements[j];
                if (((element->pos[0] + 1) * BLOCK_SIZE - shift[0] >= 0 || (element->pos[0] - 1) * BLOCK_SIZE - shift[0] <= WIDTH) &&
                    ((element->pos[1] + 1) * BLOCK_SIZE - shift[1] >= 0 || (element->pos[1] - 1) * BLOCK_SIZE - shift[1] <= HEIGHT))
                {
                    transforms[j * 4 + 0] = BLOCK_SIZE / WIDTH;
                    transforms[j * 4 + 1] = BLOCK_SIZE / HEIGHT;
                    transforms[j * 4 + 2] = 1.0 / WIDTH * element->pos[0] * BLOCK_SIZE - shift[0];
                    transforms[j * 4 + 3] = 1.0 / HEIGHT * element->pos[1] * BLOCK_SIZE - shift[1];
                    
                    switch (element->type)
                    {
                        case conductor:
                            types[j] = 1;
                            break;
                        case tail:
                            types[j] = 2;
                            break;
                        case head:
                            types[j] = 3;
                            break;
                        default:
                            break;
                    }
                    elementsCounter++;
                }
            }
        }
    }
    
    patterns[0]->spriteShader.Use();
    glUniform4fv(TransformUniformPos, elementsCounter, transforms);
    glUniform1iv(TypesUniformPos, elementsCounter, types);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, patterns[0]->EBO);
    glBindVertexArray(patterns[0]->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, patterns[0]->spriteTexture);
    glUniform1i(glGetUniformLocation(patterns[0]->spriteShader.Program, "Texture1"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, patterns[1]->spriteTexture);
    glUniform1i(glGetUniformLocation(patterns[1]->spriteShader.Program, "Texture2"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, patterns[2]->spriteTexture);
    glUniform1i(glGetUniformLocation(patterns[2]->spriteShader.Program, "Texture3"), 2);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, elementsCounter);
    glBindVertexArray(0);
}

GLFWwindow* MyInit(GLuint w, GLuint h)
{
    std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;
    // Init GLFW
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(w, h, "WireWorld_v0.0.1", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    glewInit();

    // Define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);  
    glViewport(0, 0, width, height);
    return window;
}

void initializeGame()
{
    // auto ent = createCellTemplate(0,0,400,300, "./Textures/cat.png");
    // ent->giveTag("simpleSprite");
    // createCellTemplate(400,300,400,300, "./Textures/cat.png");

    auto e = createSchemeTemplate();
    // static_cast<SchemeInfoComponent*>(e->components[0])->AddElement(conductor, 0, 0);
    // static_cast<SchemeInfoComponent*>(e->components[0])->AddElement(tail, 1, 1);
    // static_cast<SchemeInfoComponent*>(e->components[0])->AddElement(head, 2, 2);
    loadSchemeFromFile();
    createMenuUV();

    Entity* BackgroungCell = createCellTemplate(0,0,50,50, "./Textures/Blocks/empty.png", "./Code/Shaders/multiSpriteVertex.vs", "./Code/Shaders/spriteFragment.fs");
    BackgroungCell->giveTag("BackgroundSprite");
    Entity* SchemeElementsButton = new Entity();
    SchemeElementsButton->giveTag("ElementsBack");


    std::function<void(float xpos, float ypos, int key)> f = [&](float xpos, float ypos, int key)
    {
        if (key != GLFW_MOUSE_BUTTON_LEFT && key != GLFW_MOUSE_BUTTON_RIGHT)
            return;

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
        float BLOCK_SIZE = state->block_size;
        float real_pos[] = {xpos + (float)state->worldShift[0] * WIDTH, ypos + (float)state->worldShift[1] * HEIGHT};
        SchemeInfoComponent* scheme = nullptr;
        int position[2] = {(int)floor(real_pos[0]/BLOCK_SIZE), (int)floor(real_pos[1]/BLOCK_SIZE)};
        for (size_t i = 0; i < Entity::entities.size(); i++)
        {
            if (Entity::entities[i]->HasComponent(SchemeInfoComponent::GetCompId(), id))
            {
                scheme = static_cast<SchemeInfoComponent*>(Entity::entities[i]->components[id]);
                break;
            }
        }
        for (size_t i = 0; i < scheme->elements.size(); i++)
        {
            if (scheme->elements[i]->pos[0] == position[0] && scheme->elements[i]->pos[1] == position[1])
            {
                scheme->elements.erase(scheme->elements.begin() + i);
            }
        }
        if (key == GLFW_MOUSE_BUTTON_LEFT && state->chosedElement != empty)
        {
            scheme->elements.push_back(new SchemeElement(state->chosedElement, position[0], position[1]));
        }
    };
    SchemeElementsButton->components.push_back(new ButtonComponent(0,0,0,WIDTH,HEIGHT,f));


    Entity* ConductorSprite = createCellTemplate(0,0,50,50, "./Textures/Blocks/conductor.png", "./Code/Shaders/schemeSpriteVertex.vs", "./Code/Shaders/schemeSpriteFragment.fs");
    ConductorSprite->giveTag("ConductorSprite");
    Entity* TailSprite = createCellTemplate(0,0,50,50, "./Textures/Blocks/tail.png", "./Code/Shaders/schemeSpriteVertex.vs", "./Code/Shaders/schemeSpriteFragment.fs");
    TailSprite->giveTag("TailSprite");
    Entity* HeadSprite = createCellTemplate(0,0,50,50, "./Textures/Blocks/head.png", "./Code/Shaders/schemeSpriteVertex.vs", "./Code/Shaders/schemeSpriteFragment.fs");
    HeadSprite->giveTag("HeadSprite");

    createStateDescriptionTemplate();
}

void saveSchemeToFile()
{
    SchemeInfoComponent *scheme = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned id;
        if (Entity::entities[i]->HasComponent(SchemeInfoComponent::GetCompId(), id))
        {
            scheme = static_cast<SchemeInfoComponent*>(Entity::entities[i]->components[id]);
            break;
        }
    }

    std::ofstream fileOut("level.txt");
    fileOut << scheme->elements.size() << " ";
    for (size_t i = 0; i < scheme->elements.size(); i++)
    {
        fileOut << scheme->elements[i]->type << " { " << scheme->elements[i]->pos[0] << " " << scheme->elements[i]->pos[1] << " } ";
    }
    fileOut.close();
}

void loadSchemeFromFile()
{
    SchemeInfoComponent *scheme = nullptr;
    for (size_t i = 0; i < Entity::entities.size(); i++)
    {
        unsigned id;
        if (Entity::entities[i]->HasComponent(SchemeInfoComponent::GetCompId(), id))
        {
            scheme = static_cast<SchemeInfoComponent*>(Entity::entities[i]->components[id]);
            break;
        }
    }

    std::ifstream fileIn("level.txt");
    char c;
    int n;
    fileIn >> n;
    for (size_t i = 0; i < n; i++)
    {
        int type, pos[2];
        Element elem = conductor;
        fileIn >> type >> c >> pos[0] >> pos[1] >> c;
        switch (type)
        {
            case 1:
                elem = conductor;
                break;

            case 2:
                elem = tail;
                break;

            case 3:
                elem = head;
                break;

            default:
                break;
        }
        scheme->elements.push_back(new SchemeElement(elem, pos[0], pos[1]));
    }
    fileIn.close();
}

void createMenuUV()
{
    const float PANEL_WIDTH = 0.8, PANEL_HEIGHT = 0.1;
    const float BUTTON_INTERVAL_COEF = 0.4;
    Entity* PanelSprite = createCellTemplate((1 - PANEL_WIDTH) * 0.5 * WIDTH,0,
        PANEL_WIDTH * WIDTH,PANEL_HEIGHT * HEIGHT, "./Textures/Menu/button_panel.png");
    PanelSprite->giveTag("panelSprite");

    std::string pathes[] = {"./Textures/Menu/quest_button.png", "./Textures/Menu/pause_button.png",
        "./Textures/Menu/move_button.png", "./Textures/Menu/minus_button.png", "./Textures/Menu/plus_button.png",
        "./Textures/Menu/size_button.png", "./Textures/Menu/minus_button.png", "./Textures/Menu/plus_button.png",
        "./Textures/Menu/conductor_button.png", "./Textures/Menu/tail_button.png", "./Textures/Menu/head_button.png"};

    std::function<void(float xpos, float ypos, int key)> quest_f = [&](float xpos, float ypos, int key)
    {
        

        Entity* HelpPopup = createCellTemplate(0,0,WIDTH,HEIGHT, "./Textures/Menu/help.png");
        HelpPopup->giveTag("helpPopup");
        std::function<void(float xpos, float ypos, int key)> f = [&](float xpos, float ypos, int key)
        {
            unsigned id;
            for (size_t i = 0; i < Entity::entities.size(); i++)
            {
                if(Entity::entities[i]->isHaveTag("helpPopup"))
                {
                    Entity::entities.erase(Entity::entities.begin() + i);
                    break;
                }
            }
            
        };
        HelpPopup->components.push_back(new ButtonComponent(2,0,0,WIDTH,HEIGHT,f));
    };
    std::function<void(float xpos, float ypos, int key)> pause_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        state->isGamePaused = !state->isGamePaused;
    };
    std::function<void(float xpos, float ypos, int key)> empty_f = [&](float xpos, float ypos, int key)
    {};
    std::function<void(float xpos, float ypos, int key)> speeddown_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        float speedCoef = 1.5f;
        state->gameSpeed *= speedCoef;
    };
    std::function<void(float xpos, float ypos, int key)> speedup_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        float speedCoef = 1.5f;
        state->gameSpeed /= speedCoef;
    };
    std::function<void(float xpos, float ypos, int key)> sizedown_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        float sizeShift = 5.0f;
        if (state->block_size - sizeShift > state->block_size_borders[0])
        {
            state->block_size -= sizeShift;
        }
    };
    std::function<void(float xpos, float ypos, int key)> sizeup_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        float sizeShift = 5.0f;
        if (state->block_size + sizeShift < state->block_size_borders[1])
        {
            state->block_size += sizeShift;
        }
    };
    std::function<void(float xpos, float ypos, int key)> conductor_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        state->chosedElement = conductor;
    };
    std::function<void(float xpos, float ypos, int key)> tail_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        state->chosedElement = tail;
    };
    std::function<void(float xpos, float ypos, int key)> head_f = [&](float xpos, float ypos, int key)
    {
        StateDescriptionComponent* state = Entity::GetStateDescriptor();
        state->chosedElement = head;
    };

    std::function<void(float, float, int)> funcs[] = {quest_f, pause_f, empty_f, speeddown_f,
        speedup_f, empty_f, sizedown_f, sizeup_f, conductor_f, tail_f, head_f};

    int len = sizeof(pathes) / sizeof(pathes[0]);

    float button_size[] = {PANEL_WIDTH / (len + (len + 1) * BUTTON_INTERVAL_COEF), PANEL_WIDTH / (len + (len + 1) * BUTTON_INTERVAL_COEF) * WIDTH / HEIGHT};
    float starting_offset = (1 - PANEL_WIDTH) * 0.5;
        
    // std::cout << button_size[0] << " " << button_size[1] << " " << starting_offset << std::endl;
    for (int i = 0; i < len; i++)
    {
        float buttons_height = (PANEL_HEIGHT - button_size[1]) / 2;
        float p[] = {(starting_offset + (i  + (i + 1) * BUTTON_INTERVAL_COEF) * button_size[0]) * WIDTH,
                        buttons_height * HEIGHT, button_size[0] * WIDTH,button_size[1] * HEIGHT};
        auto ent = createCellTemplate(p[0], p[1], p[2], p[3], pathes[i]);
        ent->giveTag("menuSprite");
        ent->components.push_back(new ButtonComponent(1, p[0], p[1], p[2], p[3],funcs[i]));
        // std::cout << starting_offset + button_size[0] + i * (2 * button_size[0]) << " " << buttons_height << " " << button_size[0] * WIDTH << " " << button_size[1] * HEIGHT << "\n"; 
    }
}