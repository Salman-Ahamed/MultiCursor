#include "minitest.h"
#include "Core/AppStateMachine.h"

TEST(AppStateMachine, StartsAtStarting) {
    AppStateMachine sm;
    EXPECT_EQ(sm.Current(), AppState::Starting);
}

TEST(AppStateMachine, StartingToRunning) {
    AppStateMachine sm;
    EXPECT_TRUE(sm.TransitionTo(AppState::Running));
    EXPECT_EQ(sm.Current(), AppState::Running);
}

TEST(AppStateMachine, StartingToShuttingDown) {
    AppStateMachine sm;
    EXPECT_TRUE(sm.TransitionTo(AppState::ShuttingDown));
    EXPECT_EQ(sm.Current(), AppState::ShuttingDown);
}

TEST(AppStateMachine, InvalidTransitionFromStarting) {
    AppStateMachine sm;
    EXPECT_FALSE(sm.TransitionTo(AppState::Suspended));
    EXPECT_FALSE(sm.TransitionTo(AppState::DeviceLost));
    EXPECT_FALSE(sm.TransitionTo(AppState::Recovering));
    EXPECT_EQ(sm.Current(), AppState::Starting);
}

TEST(AppStateMachine, RunningTransitions) {
    AppStateMachine sm;
    sm.TransitionTo(AppState::Running);
    EXPECT_TRUE(sm.TransitionTo(AppState::Suspended));
    sm.TransitionTo(AppState::Running);
    EXPECT_TRUE(sm.TransitionTo(AppState::DeviceLost));
    EXPECT_TRUE(sm.TransitionTo(AppState::ShuttingDown));
}

TEST(AppStateMachine, InvalidFromRunning) {
    AppStateMachine sm;
    sm.TransitionTo(AppState::Running);
    EXPECT_FALSE(sm.TransitionTo(AppState::Starting));
    EXPECT_FALSE(sm.TransitionTo(AppState::Recovering));
}

TEST(AppStateMachine, SuspendedTransitions) {
    AppStateMachine sm;
    sm.TransitionTo(AppState::Running);
    sm.TransitionTo(AppState::Suspended);
    EXPECT_TRUE(sm.TransitionTo(AppState::Running));
    EXPECT_TRUE(sm.TransitionTo(AppState::ShuttingDown));
    EXPECT_FALSE(sm.TransitionTo(AppState::ShuttingDown)); // Already shutting down
}

TEST(AppStateMachine, DeviceLostChain) {
    AppStateMachine sm;
    sm.TransitionTo(AppState::Running);
    sm.TransitionTo(AppState::DeviceLost);
    EXPECT_TRUE(sm.TransitionTo(AppState::Recovering));
    EXPECT_TRUE(sm.TransitionTo(AppState::Running));
}

TEST(AppStateMachine, CallbackOnTransition) {
    AppStateMachine sm;
    AppState oldState = AppState::Starting;
    AppState newState = AppState::Starting;
    sm.OnTransition([&](AppState o, AppState n) { oldState = o; newState = n; });
    sm.TransitionTo(AppState::Running);
    EXPECT_EQ(oldState, AppState::Starting);
    EXPECT_EQ(newState, AppState::Running);
}

TEST(AppStateMachine, ShuttingDownFinal) {
    AppStateMachine sm;
    sm.TransitionTo(AppState::Running);
    sm.TransitionTo(AppState::ShuttingDown);
    EXPECT_FALSE(sm.TransitionTo(AppState::Running));
    EXPECT_FALSE(sm.TransitionTo(AppState::Suspended));
    EXPECT_FALSE(sm.TransitionTo(AppState::DeviceLost));
    EXPECT_EQ(sm.Current(), AppState::ShuttingDown);
}
