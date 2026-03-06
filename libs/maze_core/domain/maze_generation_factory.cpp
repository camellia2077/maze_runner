#include "domain/maze_generation.h"

#include <cctype>
#include <utility>

#include "domain/maze_generation_algorithms_internal.h"

namespace MazeDomain {

MazeGeneratorFactory::MazeGeneratorFactory() {
  register_generator(MazeAlgorithmType::DFS, "DFS",
                     MazeGenerationDetail::GenerateMazeDfs);
  register_generator(MazeAlgorithmType::PRIMS, "Prims",
                     MazeGenerationDetail::GenerateMazePrims);
  register_generator(MazeAlgorithmType::KRUSKAL, "Kruskal",
                     MazeGenerationDetail::GenerateMazeKruskal);
  register_generator(MazeAlgorithmType::RECURSIVE_DIVISION,
                     "Recursive Division",
                     MazeGenerationDetail::GenerateMazeRecursiveDivision);
  register_generator(MazeAlgorithmType::GROWING_TREE, "Growing Tree",
                     MazeGenerationDetail::GenerateMazeGrowingTree);
}

auto MazeGeneratorFactory::instance() -> MazeGeneratorFactory& {
  static MazeGeneratorFactory instance;
  return instance;
}

void MazeGeneratorFactory::register_generator(MazeAlgorithmType type,
                                              std::string name,
                                              Generator generator) {
  registry_[type] =
      Entry{.name = std::move(name), .generator = std::move(generator)};

  std::string key;
  key.reserve(registry_[type].name.size());
  for (unsigned char character : registry_[type].name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  name_to_type_[key] = type;
}

auto MazeGeneratorFactory::has_generator(MazeAlgorithmType type) const -> bool {
  return registry_.contains(type);
}

auto MazeGeneratorFactory::get_generator(MazeAlgorithmType type) const
    -> MazeGeneratorFactory::Generator {
  const auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.generator;
  }
  return {};
}

auto MazeGeneratorFactory::name_for(MazeAlgorithmType type) const -> std::string {
  const auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.name;
  }
  return {};
}

auto MazeGeneratorFactory::try_parse(std::string_view name,
                                     MazeAlgorithmType& out_type) const -> bool {
  std::string key;
  key.reserve(name.size());
  for (unsigned char character : name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  const auto iterator = name_to_type_.find(key);
  if (iterator == name_to_type_.end()) {
    return false;
  }
  out_type = iterator->second;
  return true;
}

auto MazeGeneratorFactory::names() const -> std::vector<std::string> {
  std::vector<std::string> result;
  result.reserve(registry_.size());
  for (const auto& entry : registry_) {
    result.push_back(entry.second.name);
  }
  return result;
}

}  // namespace MazeDomain
