#include "unleash/strategies/flexiblerollout.h"
#include "unleash/utils/murmur3hash.h"
#include <nlohmann/json.hpp>
#include <random>


namespace unleash {
FlexibleRollout::FlexibleRollout(std::string_view parameters, std::string_view constraints)
    : Strategy("flexibleRollout", constraints) {
    auto flexibleRollout_json = nlohmann::json::parse(parameters);
    m_groupId = flexibleRollout_json["groupId"].get<std::string>();
    m_rollout = (flexibleRollout_json["rollout"].type() == nlohmann::json::value_t::string)
                        ? std::stoi(flexibleRollout_json["rollout"].get<std::string>())
                        : uint32_t(flexibleRollout_json["rollout"]);
    m_stickiness = flexibleRollout_json["stickiness"].get<std::string>();
}

bool FlexibleRollout::isEnabled(const Context &context) {
    auto stickinessConfiguration = m_stickiness;
    // Choose strategy configuration
    if (stickinessConfiguration == "default") {
        if (!context.userId.empty()) stickinessConfiguration = "userId";
        else if (!context.sessionId.empty())
            stickinessConfiguration = "sessionId";
        else
            stickinessConfiguration = "random";
    }
    if (stickinessConfiguration == "userId") {
        if (context.userId.empty()) return false;
        if (normalizedMurmur3(m_groupId + ":" + context.userId) > m_rollout) return false;
    } else if (stickinessConfiguration == "sessionId") {
        if (normalizedMurmur3(m_groupId + ":" + context.sessionId) > m_rollout) return false;
    } else if (stickinessConfiguration == "random") {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 100);
        if (dist6(rng) > m_rollout) return false;
    } else {
        auto customFieldIt = context.properties.find(stickinessConfiguration);
        if (customFieldIt == context.properties.end()) return false;
        if (normalizedMurmur3(m_groupId + ":" + customFieldIt->second) > m_rollout) return false;
    }
    return meetConstraints(context);
}
}  // namespace unleash