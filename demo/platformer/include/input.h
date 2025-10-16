#ifndef INPUT_HANDLER_HEADER
#define INPUT_HANDLER_HEADER

#include "pixbench/components.h"
#include "pixbench/utils/results.h"
#include <unistd.h>
#include <unordered_map>


template <typename T>
class Input {
public:
    T value;
    int last_priority_write = 0;

    inline void writeValue(T new_value, int write_prio) {
        if (write_prio < this->last_priority_write)
            return;

        this->value = new_value;
        this->last_priority_write = write_prio;
    }

    inline void resetWrite() { this->last_priority_write = 0; }
};


typedef Input<double> AxisInput;
typedef Input<bool> ButtonInput;


#define GAMEPAD_WRITE_PRIO 2
#define KEYBOARD_WRITE_PRIO 1


class InputHandler : public ScriptComponent {
public:

    Result<VoidResult, GameError> Init(Game *game, EntityManager *entityManager, EntityID self) override {
        m_game = game;

        std::cout << "InputHandler::Init" << std::endl;

        int joypad_counts;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&joypad_counts);
        for (int i = 0; i < joypad_counts; ++i) {
            SDL_JoystickID gp_joystick_id = gamepads[i];
            SDL_Gamepad* gamepad = SDL_OpenGamepad(gp_joystick_id);
            std::cout << "i: " << i << " | gamepad: " << gamepad << std::endl;
        }
        
        return ResultOK;
    }

    double getAxisInput(std::string name) {
        if ( m_axis_inputs.find(name) == m_axis_inputs.end() )
            return 0;

        return m_axis_inputs[name].value;
    }

    double getButtonInput(std::string name) {
        if ( m_button_inputs.find(name) == m_button_inputs.end() )
            return false;

        return m_button_inputs[name].value;
    }

    Result<VoidResult, GameError> Update(double deltaTime_s, EntityManager *entityManager, EntityID self) override {

        for (auto& it : m_axis_inputs) {
            it.second.resetWrite();
        }

        for (auto& it : m_button_inputs) {
            it.second.resetWrite();
        }

        return ResultOK;
    }

    Result<VoidResult, GameError> OnEvent(SDL_Event *event, EntityManager *entityManager, EntityID self) override {
        if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
            int joypad_counts;
            SDL_JoystickID* gamepads = SDL_GetGamepads(&joypad_counts);
            for (int i = 0; i < joypad_counts; ++i) {
                SDL_JoystickID gp_joystick_id = gamepads[i];
                SDL_Gamepad* gamepad = SDL_OpenGamepad(gp_joystick_id);
                std::cout << "i: " << i << " | gamepad: " << gamepad << std::endl;
            }
        }

        if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        }

        if ( !m_game )
            return ResultOK;

        
        if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP || event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            const bool is_down = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
            if (event->gbutton.button == SDL_GAMEPAD_BUTTON_LABEL_A) {
                m_button_inputs["jump"].writeValue(is_down, 0);
            }
        }

        int numkeys;
        const bool* kb_state = SDL_GetKeyboardState(&numkeys);

        if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
                m_axis_inputs["move_x"].writeValue((double)event->gaxis.value / 32767.0, GAMEPAD_WRITE_PRIO);
            }
            if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
                m_axis_inputs["move_y"].writeValue(-(double)event->gaxis.value / 32767.0, GAMEPAD_WRITE_PRIO);
            }
        } 

        if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
            m_axis_inputs["move_x"].writeValue((double)(kb_state[SDL_SCANCODE_D] - kb_state[SDL_SCANCODE_A]), KEYBOARD_WRITE_PRIO);
            m_axis_inputs["move_y"].writeValue((double)(kb_state[SDL_SCANCODE_W] - kb_state[SDL_SCANCODE_S]), KEYBOARD_WRITE_PRIO);
        }
        

        return ResultOK;
    }
    
private:
    Game* m_game = nullptr;

    std::unordered_map<std::string, AxisInput> m_axis_inputs;
    std::unordered_map<std::string, ButtonInput> m_button_inputs;
};

#endif
