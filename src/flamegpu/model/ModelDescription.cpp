#include "flamegpu/model/ModelDescription.h"

#include "flamegpu/model/AgentDescription.h"
#include "flamegpu/model/MessageDescription.h"
#include "flamegpu/model/LayerDescription.h"
#include "flamegpu/exception/FGPUException.h"

/**
* Constructors
*/
ModelDescription::ModelDescription(const std::string &model_name)
    : model(new ModelData(model_name)) { }

bool ModelDescription::operator==(const ModelDescription& rhs) const {
    return *this->model == *rhs.model;  // Compare content is functionally the same
}
bool ModelDescription::operator!=(const ModelDescription& rhs) const {
    return !(*this == rhs);
}

/**
* Accessors
*/
AgentDescription& ModelDescription::newAgent(const std::string &agent_name) {
    if (!hasAgent(agent_name)) {
        auto rtn = std::shared_ptr<AgentData>(new AgentData(model, agent_name));
        model->agents.emplace(agent_name, rtn);
        return *rtn->description;
    }
    THROW InvalidAgentName("Agent with name '%s' already exists, "
        "in ModelDescription::newAgent().",
        agent_name.c_str());
}
AgentDescription& ModelDescription::Agent(const std::string &agent_name) {
    auto rtn = model->agents.find(agent_name);
    if (rtn != model->agents.end())
        return *rtn->second->description;
    THROW InvalidAgentName("Agent ('%s') was not found, "
        "in ModelDescription::Agent().",
        agent_name.c_str());
}

MessageDescription& ModelDescription::newMessage(const std::string &message_name) {
    if (!hasMessage(message_name)) {
        auto rtn = std::shared_ptr<MessageData>(new MessageData(model, message_name));
        model->messages.emplace(message_name, rtn);
        return *rtn->description;
    }
    THROW InvalidMessageName("Message with name '%s' already exists, "
        "in ModelDescription::newMessage().",
        message_name.c_str());
}
template<typename TDesc, typename TData>
TDesc& ModelDescription::newMessage(const std::string &message_name) {
    if (!hasMessage(message_name)) {
        auto rtn = std::shared_ptr<TData>(new TData(model, message_name));
        model->messages.emplace(message_name, rtn);
        return *reinterpret_cast<TDesc*>(rtn->description.get());
    }
    THROW InvalidMessageName("Message with name '%s' already exists, "
        "in ModelDescription::newMessage().",
        message_name.c_str());
}
Spatial2DMessageDescription& ModelDescription::newSpatial2DMessage(const std::string &message_name) {
    return newMessage<Spatial2DMessageDescription, Spatial2DMessageData>(message_name);
}
Spatial3DMessageDescription& ModelDescription::newSpatial3DMessage(const std::string &message_name) {
    return newMessage<Spatial3DMessageDescription, Spatial3DMessageData>(message_name);
}
MessageDescription& ModelDescription::Message(const std::string &message_name) {
    auto rtn = model->messages.find(message_name);
    if (rtn != model->messages.end())
        return *rtn->second->description;
    THROW InvalidMessageName("Message ('%s') was not found, "
        "in ModelDescription::Message().",
        message_name.c_str());
}

template<typename TDesc, typename TData>
TDesc& ModelDescription::Message(const std::string &message_name) {
    auto rtn = model->messages.find(message_name);
    if (rtn != model->messages.end()) {
        if (auto r = std::dynamic_pointer_cast<TData>(rtn->second)) {
            return *reinterpret_cast<TDesc*>(r->description.get());
        }
        THROW InvalidMessageName("Message ('%s') is not of correct type, "
            "in ModelDescription::Message().",
            message_name.c_str());
    }
    THROW InvalidMessageName("Message ('%s') was not found, "
        "in ModelDescription::Message().",
        message_name.c_str());
}
Spatial2DMessageDescription& ModelDescription::Spatial2DMessage(const std::string &message_name) {
    return Message<Spatial2DMessageDescription, Spatial2DMessageData>(message_name);
}
Spatial3DMessageDescription& ModelDescription::Spatial3DMessage(const std::string &message_name) {
    return Message<Spatial3DMessageDescription, Spatial3DMessageData>(message_name);
}

LayerDescription& ModelDescription::newLayer(const std::string &name) {
    // Ensure name is unique
    if (!name.empty()) {
        for (auto it = model->layers.begin(); it != model->layers.end(); ++it) {
            if ((*it)->name == name) {
                THROW InvalidFuncLayerIndx("Layer ('%s') already exists, "
                    "in ModelDescription::newLayer().",
                    name.c_str());
            }
        }
    }
    auto rtn = std::shared_ptr<LayerData>(new LayerData(model, name, static_cast<unsigned int>(model->layers.size())));
    model->layers.push_back(rtn);
    return *rtn->description;
}
LayerDescription& ModelDescription::Layer(const ModelData::size_type &layer_index) {
    if (model->layers.size() > layer_index) {
        auto it = model->layers.begin();
        for (auto i = 0u; i < layer_index; ++i)
            ++it;
        return *(*it)->description;
    }
    THROW OutOfBoundsException("Layer %d is out of bounds, "
        "in ModelDescription::Layer().",
        layer_index);
}
LayerDescription& ModelDescription::Layer(const std::string &name) {
    if (!name.empty()) {  // Can't search for no name, multiple layers might be nameless
            for (auto it = model->layers.begin(); it != model->layers.end(); ++it) {
                if ((*it)->name == name)
                    return *(*it)->description;
            }
    }
    THROW InvalidFuncLayerIndx("Layer '%s' was not found, "
        "in ModelDescription::Layer().",
        name.c_str());
}

void ModelDescription::addInitFunction(FLAMEGPU_INIT_FUNCTION_POINTER func_p) {
    if (!model->initFunctions.insert(func_p).second) {
        THROW InvalidHostFunc("Attempted to add same init function twice,"
            "in ModelDescription::addInitFunction()");
    }
}
void ModelDescription::addStepFunction(FLAMEGPU_STEP_FUNCTION_POINTER func_p) {
    if (!model->stepFunctions.insert(func_p).second) {
        THROW InvalidHostFunc("Attempted to add same step function twice,"
            "in ModelDescription::addStepFunction()");
    }
}
void ModelDescription::addExitFunction(FLAMEGPU_EXIT_FUNCTION_POINTER func_p) {
    if (!model->exitFunctions.insert(func_p).second) {
        THROW InvalidHostFunc("Attempted to add same exit function twice,"
            "in ModelDescription::addExitFunction()");
    }
}
void ModelDescription::addExitCondition(FLAMEGPU_EXIT_CONDITION_POINTER func_p) {
    if (!model->exitConditions.insert(func_p).second) {
        THROW InvalidHostFunc("Attempted to add same exit condition twice,"
            "in ModelDescription::addExitCondition()");
    }
}

EnvironmentDescription& ModelDescription::Environment() {
    return *model->environment;
}

/**
* Const Accessors
*/
std::string ModelDescription::getName() const {
    return model->name;
}

const AgentDescription& ModelDescription::getAgent(const std::string &agent_name) const {
    auto rtn = model->agents.find(agent_name);
    if (rtn != model->agents.end())
        return *rtn->second->description;
    THROW InvalidAgentVar("Agent ('%s') was not found, "
        "in ModelDescription::getAgent().",
        agent_name.c_str());
}
const MessageDescription& ModelDescription::getMessage(const std::string &message_name) const {
    auto rtn = model->messages.find(message_name);
    if (rtn != model->messages.end())
        return *rtn->second->description;
    THROW InvalidMessageVar("Message ('%s') was not found, "
        "in ModelDescription::getMessage().",
        message_name.c_str());
}

template<typename TDesc, typename TData>
const TDesc& ModelDescription::getMessage(const std::string &message_name) const {
    auto rtn = model->messages.find(message_name);
    if (rtn != model->messages.end()) {
        if (auto r = std::dynamic_pointer_cast<TData>(rtn->second)) {
            return *reinterpret_cast<TDesc*>(r->description.get());
        }
        THROW InvalidMessageName("Message ('%s') is not of correct type, "
            "in ModelDescription::getMessage().",
            message_name.c_str());
    }
    THROW InvalidMessageName("Message ('%s') was not found, "
        "in ModelDescription::getMessage().",
        message_name.c_str());
}

const Spatial2DMessageDescription& ModelDescription::getSpatial2DMessage(const std::string &message_name) const {
    return getMessage<Spatial2DMessageDescription, Spatial2DMessageData>(message_name);
}
const Spatial3DMessageDescription& ModelDescription::getSpatial3DMessage(const std::string &message_name) const {
    auto rtn = model->messages.find(message_name);
    if (rtn != model->messages.end()) {
        if (auto r = std::dynamic_pointer_cast<Spatial3DMessageData>(rtn->second)) {
            return *reinterpret_cast<Spatial3DMessageDescription*>(r->description.get());
        }
        THROW InvalidMessageName("Message ('%s') is not of type Spatial2D, "
            "in ModelDescription::getSpatial3DMessage().",
            message_name.c_str());
    }
    THROW InvalidMessageName("Message ('%s') was not found, "
        "in ModelDescription::getSpatial3DMessage().",
        message_name.c_str());
}
const EnvironmentDescription& ModelDescription::getEnvironment() const {
    return *model->environment;
}
const LayerDescription& ModelDescription::getLayer(const std::string &name) const {
    if (!name.empty()) {  // Can't search for no name, multiple layers might be nameless
        for (auto it = model->layers.begin(); it != model->layers.end(); ++it) {
            if ((*it)->name == name)
                return *(*it)->description;
        }
    }
    THROW InvalidFuncLayerIndx("Layer ('%s') was not found, "
        "in ModelDescription::getAgent().",
        name.c_str());
}
const LayerDescription& ModelDescription::getLayer(const ModelData::size_type &layer_index) const {
    if (model->layers.size() > layer_index) {
        auto it = model->layers.begin();
        for (auto i = 0u; i < layer_index; ++i)
            ++it;
        return *(*it)->description;
    }
    THROW OutOfBoundsException("Layer %d is out of bounds, "
        "in ModelDescription::Layer().",
        layer_index);
}
bool ModelDescription::hasAgent(const std::string &agent_name) const {
    return model->agents.find(agent_name) != model->agents.end();
}
bool ModelDescription::hasMessage(const std::string &message_name) const {
    return model->messages.find(message_name) != model->messages.end();
}
template<typename TData>
bool ModelDescription::hasMessage(const std::string &message_name) const {
    auto a = model->messages.find(message_name);
    if (a != model->messages.end()) {
        if (std::dynamic_pointer_cast<TData>(a->second))
            return true;
    }
    return false;
}
bool ModelDescription::hasSpatial2DMessage(const std::string &message_name) const {
    return hasMessage<Spatial2DMessageData>(message_name);
}
bool ModelDescription::hasSpatial3DMessage(const std::string &message_name) const {
    return hasMessage<Spatial3DMessageData>(message_name);
}
bool ModelDescription::hasLayer(const std::string &name) const {
    if (!name.empty()) {  // Can't search for no name, multiple layers might be nameless
        for (auto it = model->layers.begin(); it != model->layers.end(); ++it) {
            if ((*it)->name == name)
                return true;
        }
    }
    return false;
}
bool ModelDescription::hasLayer(const ModelData::size_type &layer_index) const {
    return layer_index < model->layers.size();
}

ModelData::size_type ModelDescription::getAgentsCount() const {
    // This down-cast is safe
    return static_cast<ModelData::size_type>(model->agents.size());
}
ModelData::size_type ModelDescription::getMessagesCount() const {
    // This down-cast is safe
    return static_cast<ModelData::size_type>(model->messages.size());
}
ModelData::size_type ModelDescription::getLayersCount() const {
    // This down-cast is safe
    return static_cast<ModelData::size_type>(model->layers.size());
}