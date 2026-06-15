#include "AppStateMachine.h"

bool AppStateMachine::IsValid(AppState from, AppState to) {
    switch (from) {
    case AppState::Starting:
        return to == AppState::Running || to == AppState::ShuttingDown;
    case AppState::Running:
        return to == AppState::Suspended || to == AppState::DeviceLost ||
               to == AppState::ShuttingDown;
    case AppState::Suspended:
        return to == AppState::Running || to == AppState::ShuttingDown;
    case AppState::DeviceLost:
        return to == AppState::Recovering || to == AppState::ShuttingDown;
    case AppState::Recovering:
        return to == AppState::Running || to == AppState::DeviceLost ||
               to == AppState::ShuttingDown;
    case AppState::ShuttingDown:
        return false;
    }
    return false;
}
