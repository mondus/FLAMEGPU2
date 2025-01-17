#include "flamegpu/flamegpu.h"

#include "gtest/gtest.h"


namespace flamegpu {
namespace test_host_api {

// Test host_api::getStepCounter()

FLAMEGPU_AGENT_FUNCTION(agent_function_getStepCounter, MessageNone, MessageNone) {
    FLAMEGPU->setVariable<unsigned int>("step", FLAMEGPU->getStepCounter());
    return ALIVE;
}

const unsigned int TOTAL_STEPS = 4;

// globally scoped variable to track the value in the test?
unsigned int expectedStepCounter = 0;

// Init should always be 0th iteration/step
FLAMEGPU_INIT_FUNCTION(init_testGetStepCounter) {
    EXPECT_EQ(FLAMEGPU->getStepCounter(), 0u);
}
// host is during, so 0? - @todo dynamic
FLAMEGPU_HOST_FUNCTION(host_testGetStepCounter) {
    EXPECT_EQ(FLAMEGPU->getStepCounter(), expectedStepCounter);
}
// Step functions are at the end of the step
FLAMEGPU_STEP_FUNCTION(step_testGetStepCounter) {
    EXPECT_EQ(FLAMEGPU->getStepCounter(), expectedStepCounter);
}

// Runs between steps - i.e. after step functions
FLAMEGPU_EXIT_CONDITION(exitCondition_testGetStepCounter) {
    EXPECT_EQ(FLAMEGPU->getStepCounter(), expectedStepCounter);
    // Increment the counter used for testing multiple steps.
    expectedStepCounter++;
    return CONTINUE;
}

// exit is after all iterations, so stepCounter is the total number executed.
FLAMEGPU_EXIT_FUNCTION(exit_testGetStepCounter) {
    EXPECT_EQ(FLAMEGPU->getStepCounter(), TOTAL_STEPS);
}

TEST(hostAPITest, getStepCounter) {
    ModelDescription model("model");
    AgentDescription &agent = model.newAgent("agent");

    model.addInitFunction(init_testGetStepCounter);
    model.newLayer().addHostFunction(host_testGetStepCounter);
    model.addStepFunction(step_testGetStepCounter);
    model.addExitCondition(exitCondition_testGetStepCounter);
    model.addExitFunction(exit_testGetStepCounter);

    // Init pop
    const unsigned int agentCount = 1;
    AgentVector init_population(agent, agentCount);
    // Setup Model
    CUDASimulation cudaSimulation(model);
    cudaSimulation.setPopulationData(init_population);

    cudaSimulation.SimulationConfig().steps = TOTAL_STEPS;
    cudaSimulation.simulate();
}

FLAMEGPU_STEP_FUNCTION(reduce_value) {
    // Perform a reduction
    EXPECT_NO_THROW(FLAMEGPU->agent("foo").sum<float>("bar"));
}
FLAMEGPU_AGENT_FUNCTION(birth_agent, MessageNone, MessageNone) {
    FLAMEGPU->agent_out.setVariable<float>("bar", 1.0f);
    return ALIVE;
}
TEST(hostAPITest, resizeTempMemory) {
    // This test is an attempt to catch bugs inside HostAPI::tempStorageRequiresResize()
    // e.g. if we don't resize enough memory before calling cub
    ModelDescription model("model");
    AgentDescription &agent = model.newAgent("foo");
    agent.newVariable<float>("bar");

    auto& afn = agent.newFunction("birth_agent", birth_agent);
    afn.setAgentOutput(agent);

    model.newLayer().addAgentFunction(afn);
    model.addStepFunction(reduce_value);

    // Init pop
    const unsigned int agentCount = 1;
    AgentVector init_population(agent, agentCount);
    // Setup Model
    CUDASimulation cudaSimulation(model);
    cudaSimulation.setPopulationData(init_population);

    cudaSimulation.SimulationConfig().steps = 16;
    cudaSimulation.simulate();
}

}  // namespace test_host_api
}  // namespace flamegpu
