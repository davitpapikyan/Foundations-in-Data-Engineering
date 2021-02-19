#ifndef FDE20_BONUSPROJECT_3_KNN_HPP
#define FDE20_BONUSPROJECT_3_KNN_HPP

#include "Matrix.hpp"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <vector>


bool cmp(Matrix::Entry& a, Matrix::Entry& b)
{
    return a.weight < b.weight;
}


//---------------------------------------------------------------------------
/// Find the top k neighbors for the node start. The directed graph is stored in
/// matrix m and you can use getNeighbors(node) to get all neighbors of a node.
/// A more detailed description is provided in Matrix.hpp.
/// The function should return the k nearest neighbors in sorted order as vector
/// of Matrix::Entry, where Entry->column is the neighbor node and Entry->weight
/// is the cost to reach the node from the start node.
std::vector<Matrix::Entry> getKNN(const Matrix &m, unsigned start, unsigned k) {
  using Entry = Matrix::Entry;
  std::vector<Entry> result;
  result.reserve(k);

  std::unordered_map<unsigned, double> node2distance;  // Stores node as key and distance from start as value.
  std::unordered_set<unsigned> toVisit;  // Stores nodes for visiting next.
  double d {};

  // Initializing node2distance and toVisit.
  for (auto &node : m.getNeighbors(start)) {
      toVisit.insert(node.column);
      node2distance.insert({node.column, node.weight});
  }

  while (!toVisit.empty()){
      for (auto node = toVisit.begin(); node != toVisit.end(); ){
              for (auto &neighbor : m.getNeighbors(*node)) {
                  if (neighbor.column != start){
                      // If neighbor has not been examined, then add its distance information and save it to visit.
                      if (node2distance.find(neighbor.column) == node2distance.end()){
                          toVisit.insert(neighbor.column);
                          node2distance.insert({neighbor.column, node2distance[*node] + neighbor.weight});
                      }
                      else{
                          // If neighbor has already been examined, then check if the current distance is lower than
                          // the observed one and make corresponding changes.
                          d = node2distance[*node] + neighbor.weight;  // New distance.
                          if (node2distance[neighbor.column] > d){
                              node2distance[neighbor.column] = d;
                              // Since we found a cheaper way to get to neighbor, we need to explore its neighbors again.
                              toVisit.insert(neighbor.column);
                          }
                      }
                  }
              }
              node = toVisit.erase(node);  // Deleting the node which was visited.
      }
  }

  for (auto& it : node2distance) result.push_back(Matrix::Entry(it.first, it.second));
  sort(result.begin(), result.end(), cmp);
  // Keep only top k nodes if at least k neighbors exist, otherwise keep found neighbors and cut off the remaining part.
  result = std::vector<Matrix::Entry>(result.begin(), result.begin() + std::min<unsigned>(result.size(), k));
  return result;
}

//---------------------------------------------------------------------------

#endif // FDE20_BONUSPROJECT_3_KNN_HPP
