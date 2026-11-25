#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

inline constexpr int kSamplesPerSeg = 24;
inline constexpr float kTrackWidth = 60.0f; // pixels
inline constexpr float kWorldScale = 1.0f / 50.0f;

// Match the track placement in config.cfg and the car scale in car.anim.
inline constexpr float kTrackSceneScale = 6.0f;
inline constexpr float kTrackSceneHeight = -0.9f;
inline constexpr float kCarScale = 0.12f;

glm::vec2 bsplinePoint(const glm::vec2 &p0, const glm::vec2 &p1,
                       const glm::vec2 &p2, const glm::vec2 &p3, float t);
std::vector<glm::vec2> evalBSpline(const std::vector<glm::vec2> &pts, bool closed);
void buildOffsets(const std::vector<glm::vec2> &center, bool closed,
                  std::vector<glm::vec2> &inner, std::vector<glm::vec2> &outer);

glm::vec2 trackCentroid(const std::vector<glm::vec2> &center);
glm::vec3 trackPointToWorld(const glm::vec2 &point, const glm::vec2 &centroid);

// One Tx Ty Tz Sx Sy Sz Rx Ry Rz record per centerline sample.
bool writeTrackAnimation(std::ostream &out, const std::vector<glm::vec2> &center,
                         const glm::vec2 &centroid, bool closed);
bool writeTrackAnimation(const std::string &path, const std::vector<glm::vec2> &center,
                         const glm::vec2 &centroid, bool closed);
