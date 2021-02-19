#include "JoinQuery.hpp"
#include <assert.h>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string>
#include<unordered_set>


// Converts a string of digits into unsigned integer.
unsigned toInt(const char *start, const char *end) {
   unsigned value {};
   for (auto iter=start; iter < end; ++iter)
      value = value * 10 + (*iter - '0');
   return value;
}

//---------------------------------------------------------------------------
JoinQuery::JoinQuery(std::string lineitem, std::string order,
                     std::string customer)
{
   // Storing files' paths.
   lineitem_path = lineitem;
   orders_path = order;
   customer_path = customer;
}
//---------------------------------------------------------------------------
size_t JoinQuery::avg(std::string segmentParam)
{
   // Mapping 'customer.tbl' into memory.
   // Reference to Prof. Dr. Thomas Neumann.
   int handle_c = open(customer_path.c_str(), O_RDONLY);
   lseek(handle_c, 0, SEEK_END);
   auto length_c = lseek(handle_c, 0, SEEK_CUR);
   void* data_c = mmap(nullptr, length_c, PROT_READ, MAP_SHARED, handle_c, 0);
   auto begin_c = static_cast<const char *>(data_c);
   auto end_c = begin_c + length_c;

   // Extracts all 'custkey's filtered by 'segmentParam' and stores them into unordered set custKeys.
   std::unordered_set<unsigned> custKeys;
   for (auto iter = begin_c; iter < end_c;) {
      const char *last = nullptr;
      const char *newline = iter;
      const char *endKey = nullptr;
      unsigned n_col {};
      for (; iter < end_c; ++iter)
         if ((*iter) == '|') {
            ++n_col;
            if (n_col == 1){
               endKey = iter;
            }
            else {
               if (n_col == 6) {
                  last = iter + 1;
               } else if (n_col == 7) {
                  std::string segment = std::string(last, iter);
                  if (segment == segmentParam){
                     custKeys.insert(toInt(newline, endKey));
                  }
                  while ((iter < end_c) && ((*iter) != '\n')) ++iter;
                  if (iter < end_c) ++iter;
                  break;
               }
            }
         }
   }
   munmap(data_c, length_c);
   close(handle_c);

   // Mapping 'orders.tbl' into memory.
   // Reference to Prof. Dr. Thomas Neumann.
   int handle_o = open(orders_path.c_str(), O_RDONLY);
   lseek(handle_o, 0, SEEK_END);
   auto length_o = lseek(handle_o, 0, SEEK_CUR);
   void* data_o = mmap(nullptr, length_o, PROT_READ, MAP_SHARED, handle_o, 0);
   auto begin_o = static_cast<const char *>(data_o);
   auto end_o = begin_o + length_o;

   // Stores 'orderkey's corresponding to 'custkey's into unordered set orderKeys.
   std::unordered_set<unsigned> orderKeys;
   for (auto iter = begin_o; iter < end_o;) {
      const char *last = nullptr;
      const char *newline = iter;
      const char *endKey = nullptr;
      unsigned n_col {};
      for (; iter < end_o; ++iter)
         if ((*iter) == '|') {
            ++n_col;
            if (n_col == 1){
               endKey = iter;
               last = iter + 1;
            }
            else if (n_col == 2) {
               unsigned custKey {toInt(last, iter)};
               if (custKeys.find(custKey) != custKeys.end()) {
                  orderKeys.insert(toInt(newline, endKey));
               }
               while ((iter < end_o) && ((*iter) != '\n')) ++iter;
               if (iter < end_o) ++iter;
               break;
            }
         }
   }
   munmap(data_o, length_o);
   close(handle_o);

   // Mapping 'lineitem.tbl' into memory.
   // Reference to Prof. Dr. Thomas Neumann.
   int handle_l = open(lineitem_path.c_str(), O_RDONLY);
   lseek(handle_l, 0, SEEK_END);
   auto length_l = lseek(handle_l, 0, SEEK_CUR);
   void *data_l = mmap(nullptr, length_l, PROT_READ, MAP_SHARED, handle_l, 0);
   auto begin_l = static_cast<const char *>(data_l);
   auto end_l = begin_l + length_l;

   // Computes the average of 'quantity' values with corresponding 'orderkey' in orderKeys.
   double long average {};
   unsigned count {};
   for (auto iter = begin_l; iter < end_l;) {
      new_line:  // Label for goto.
      const char *last = nullptr;
      const char *newline = iter;
      unsigned n_col {};
      for (; iter < end_l; ++iter) {
         if ((*iter) == '|') {
            ++n_col;
            if (n_col == 1) {
               unsigned orderKey {toInt(newline, iter)};
               if (orderKeys.find(orderKey) != orderKeys.end()) {
                  ++iter;
                  for (; iter < end_l; ++iter) {
                     if ((*iter) == '|') {
                        ++n_col;
                        if (n_col == 4) {
                           last = iter + 1;
                        } else if (n_col == 5) {
                           average += toInt(last, iter);
                           ++count;
                           while ((iter < end_l) && ((*iter) != '\n')) ++iter;
                           if (iter < end_l) ++iter;
                           goto new_line;  // If the newline has been reached, go to the beginning of the loop.
                        }
                     }
                  }
               } else {
                  while ((iter < end_l) && ((*iter) != '\n')) ++iter;
                  if (iter < end_l) ++iter;
                  break;
               }
            }
         }
      }
   }
   munmap(data_l, length_l);
   close(handle_l);
   average /= static_cast<double>(count);
   return average * 100;
}
//---------------------------------------------------------------------------
size_t JoinQuery::lineCount(std::string rel)
{
   std::ifstream relation(rel);
   assert(relation);  // make sure the provided string references a file
   size_t n = 0;
   for (std::string line; std::getline(relation, line);) n++;
   return n;
}
//---------------------------------------------------------------------------
