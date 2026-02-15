#pragma once

#include <string>
#include <vector>

namespace overwatch {

/**
 * Represents a single search query
 */
struct Query {
    int id;
    std::string name;
    std::string query;           // GitHub search query string
    std::vector<std::string> tags;
    int max_repos;
};

/**
 * Manages a collection of queries
 */
class QueryBank {
public:
    QueryBank();

    /**
     * Load queries from YAML file
     */
    void load(const std::string& yaml_path);

    /**
     * Save queries to YAML file
     */
    void save(const std::string& yaml_path);

    /**
     * Add a new query to the bank
     */
    void addQuery(const Query& query);

    /**
     * Delete a query by ID
     */
    bool deleteQuery(int id);

    /**
     * Get all queries
     */
    std::vector<Query> getAllQueries() const;

    /**
     * Get a random query
     */
    Query getRandomQuery() const;

    /**
     * Filter queries by tag
     */
    std::vector<Query> filterByTag(const std::string& tag) const;

    /**
     * Get next available ID
     */
    int getNextId() const;

private:
    std::vector<Query> queries_;
    std::string yaml_path_;
};

} // namespace overwatch
