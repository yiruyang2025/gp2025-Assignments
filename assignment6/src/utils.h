#pragma once
#define PI 3.14159265358979323846
#include <Eigen/Eigen>

namespace utils {

/**
 * @brief Finds the data directory by checking a list of possible directories.
 *
 * @return std::string The path to the data directory.
 * @throws std::runtime_error If the data directory is not found.
 */
std::string find_data_dir();

/**
 * @brief Struct to hold mesh data.
 */
struct MeshData {
  Eigen::MatrixXd V, VN, C, UV;
  Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> texture_R,
      texture_G, texture_B, texture_A;
  bool has_texture;
  Eigen::MatrixXi F, F_UV, FN;
};

/**
 * @brief Generates a mesh for given line segments defined by the head and tail
 * vertices.
 *
 * @param V_head  #Lines x 3 First vertex of each line segments.
 * @param V_tail  #Lines x 3 Second vertex of each line segments.
 * @param radius  Radius of the cylinder.
 * @param res_x   Resolution of the 'u' parameter.
 * @param res_y   Resolution of the 'v' parameter.
 * @param C       Optional color per line segment.
 * @param shrink  Scale factor for the tail vertices. 1.0 makes a perfect
 *                cylinder, <1 makes the radius shrink towards the tail.
 * @return MeshData
 */
MeshData skeleton_mesh(const Eigen::MatrixXd &V_head,
                       const Eigen::MatrixXd &V_tail, double radius = 0.03,
                       int res_x = 10, int res_y = 10,
                       const Eigen::MatrixXd &C = Eigen::MatrixXd(),
                       double shrink = 0.5);

/**
 * @brief Generates a mesh for given line segments defined by a list of vertices
 *        and a list of edges.
 */
inline MeshData edges_mesh(const Eigen::MatrixXd &V, const Eigen::MatrixXi &E,
                           double radius = 0.03, int res_x = 10, int res_y = 10,
                           const Eigen::MatrixXd &C = Eigen::MatrixXd(),
                           double shrink = 0.5) {
  return skeleton_mesh(V(E.col(0), Eigen::all), V(E.col(1), Eigen::all), radius,
                       res_x, res_y, C, shrink);
}

/**
 * @brief Loads a skeleton from a file.
 *
 * @param filename The name of the file to load the skeleton from.
 * @param bones    #Bones x 6 The matrix to store the bone data (head and tail).
 * @param bones_parents #Childs x 2 The matrix to store the parent-child
 * relationships.
 */
void load_skeleton(const std::string &filename, Eigen::MatrixXd &bones,
                   Eigen::MatrixXi &bones_parents);

/**
 * @brief Loads a mesh from an OBJ file.
 *
 * @param filename The name of the OBJ file to load.
 * @param texture_filename The name of the texture file to load (optional).
 *                        If not provided, no texture will be loaded.
 * @return MeshData
 */
MeshData load_obj(const std::string &filename,
                  const std::string &texture_filename = "");
} // namespace utils