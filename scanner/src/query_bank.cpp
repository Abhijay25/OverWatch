#include "query_bank.h"
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <random>

namespace overwatch {

QueryBank::QueryBank() {
}

void QueryBank::load(const std::string& yaml_path) {
    yaml_path_ = yaml_path;
    spdlog::info("Loading query bank from: {}", yaml_path);

    try {
        YAML::Node config = YAML::LoadFile(yaml_path);

        if (!config["queries"]) {
            spdlog::warn("No 'queries' key found in YAML - creating empty bank");
            return;
        }

        queries_.clear();

        for (const auto& query_node : config["queries"]) {
            Query query;
            query.id = query_node["id"].as<int>();
            query.name = query_node["name"].as<std::string>();
            query.query = query_node["query"].as<std::string>();
            query.max_repos = query_node["max_repos"].as<int>();

            // Load tags
            if (query_node["tags"]) {
                for (const auto& tag : query_node["tags"]) {
                    query.tags.push_back(tag.as<std::string>());
                }
            }

            queries_.push_back(query);
        }

        spdlog::info("Loaded {} queries from bank", queries_.size());

    } catch (const YAML::Exception& e) {
        spdlog::error("Failed to load query bank: {}", e.what());
        spdlog::warn("Starting with empty query bank");
    }
}

void QueryBank::save(const std::string& yaml_path) {
    spdlog::info("Saving query bank to: {}", yaml_path);

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "queries";
    out << YAML::Value << YAML::BeginSeq;

    for (const auto& query : queries_) {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << query.id;
        out << YAML::Key << "name" << YAML::Value << query.name;
        out << YAML::Key << "query" << YAML::Value << query.query;
        out << YAML::Key << "max_repos" << YAML::Value << query.max_repos;

        out << YAML::Key << "tags" << YAML::Value << YAML::BeginSeq;
        for (const auto& tag : query.tags) {
            out << tag;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream file(yaml_path);
    file << out.c_str();
    file.close();

    spdlog::info("Saved {} queries", queries_.size());
}

void QueryBank::addQuery(const Query& query) {
    // Check for duplicate query string
    for (const auto& existing : queries_) {
        if (existing.query == query.query) {
            spdlog::warn("Duplicate query detected: '{}' (already exists as ID: {})", query.query, existing.id);
            spdlog::warn("Skipping addition of duplicate query");
            return;
        }
    }

    queries_.push_back(query);
    spdlog::info("Added query: {} (ID: {})", query.name, query.id);
}

bool QueryBank::deleteQuery(int id) {
    auto it = std::remove_if(queries_.begin(), queries_.end(),
                             [id](const Query& q) { return q.id == id; });

    if (it != queries_.end()) {
        queries_.erase(it, queries_.end());
        spdlog::info("Deleted query with ID: {}", id);
        return true;
    }

    spdlog::warn("Query with ID {} not found", id);
    return false;
}

std::vector<Query> QueryBank::getAllQueries() const {
    return queries_;
}

Query QueryBank::getRandomQuery() const {
    if (queries_.empty()) {
        throw std::runtime_error("Query bank is empty");
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, queries_.size() - 1);

    return queries_[dis(gen)];
}

std::vector<Query> QueryBank::filterByTag(const std::string& tag) const {
    std::vector<Query> result;

    for (const auto& query : queries_) {
        // Check if query has this tag
        if (std::find(query.tags.begin(), query.tags.end(), tag) != query.tags.end()) {
            result.push_back(query);
        }
    }

    return result;
}

int QueryBank::getNextId() const {
    if (queries_.empty()) {
        return 1;
    }

    int max_id = 0;
    for (const auto& query : queries_) {
        if (query.id > max_id) {
            max_id = query.id;
        }
    }

    return max_id + 1;
}

} // namespace overwatch
