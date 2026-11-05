#include <redland.h>

#include <doctest/doctest.h>

TEST_CASE("redland link smoke") {
  librdf_world* world = librdf_new_world();
  REQUIRE(world != nullptr);
  librdf_world_open(world);

  librdf_storage* storage = librdf_new_storage(world, "memory", nullptr, nullptr);
  REQUIRE(storage != nullptr);

  librdf_model* model = librdf_new_model(world, storage, nullptr);
  REQUIRE(model != nullptr);

  librdf_free_model(model);
  librdf_free_storage(storage);
  librdf_free_world(world);
}
