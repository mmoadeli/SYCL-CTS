/*******************************************************************************
//
//  SYCL 2020 Conformance Test Suite
//
//  Copyright (c) 2018-2022 Codeplay Software LTD. All Rights Reserved.
//  Copyright (c) 2022 The Khronos Group Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
*******************************************************************************/
#include "../common/common.h"
#include <algorithm>
#include <string>

namespace h_item_api {
using namespace sycl_cts;

template <int dims>
struct kernel_common;

/** @brief Storage for linear ID values
 */
struct work_item_ids {
  size_t global;
  size_t physical;
  size_t logical;
};

/** @brief Wrapper for global ID support in generic code
 */
struct global_id {
  static const auto& value(const work_item_ids& ids) { return ids.global; }
  static std::string description() { return "global ID"; };
};

/** @brief Wrapper for physical local ID support in generic code
 */
struct physical_id {
  static const auto& value(const work_item_ids& ids) { return ids.physical; }
  static std::string description() { return "physical local ID"; };
};

/** @brief Wrapper for logical local ID support in generic code
 */
struct logical_id {
  static const auto& value(const work_item_ids& ids) { return ids.logical; }
  static std::string description() { return "logical local ID"; };
};

/** @brief Provides offset logic for the test
 */
class offset_helper {
  const size_t numPerGroup;
  const size_t numTotal;

 public:
  offset_helper(size_t numLogicalPerGroup, size_t numLogicalWorkItems)
      : numPerGroup(numLogicalPerGroup), numTotal(numLogicalWorkItems) {}

  size_t total() const { return numTotal; }
  size_t max() const { return numTotal - 1; }

  template <int dims>
  size_t get(const sycl::group<dims>& group,
             const sycl::h_item<dims>& item) const {
    /**
     * There is a data race in case any of these calls return same results for
     * different work-items. As data race is UB, which has no formal guarantees
     * according to the C++ sec, there is slight possibility of missing or
     * broken output from test for such case.
     *
     * See PR#124 and 'How to Miscompile Programs with "Benign" Data Races' from
     * Hans Boehm for more details.
     */
    return group.get_linear_id() * numPerGroup +
           item.get_logical_local().get_linear_id();
  }

  size_t get_group_id(size_t offset) const { return offset / numPerGroup; }

  size_t get_logical_local_id(size_t offset) const {
    return offset % numPerGroup;
  }

  template <int dims>
  std::string to_string(size_t offset) const {
    std::string result;
    result += std::to_string(dims) + " dimensions, group: ";
    result += std::to_string(get_group_id(offset));
    result += ", logical local id: ";
    result += std::to_string(get_logical_local_id(offset));
    return result;
  }
};

struct getter {
  enum class methods_1d : size_t {
    get_global_range = 0,
    get_global_id,
    get_local_range,
    get_local_id,
    get_logical_local_range,
    get_logical_local_id,
    get_physical_local_range,
    get_physical_local_id,
    methods_count
  };

  enum class methods_nd : size_t {
    local_item = 0,
    global_range,
    local_range,
    logical_local_range,
    physical_local_range,
    global_id,
    local_id,
    logical_local_id,
    physical_local_id,
    methods_count
  };

  static constexpr auto method_cnt_1d = to_integral(methods_1d::methods_count);
  static constexpr auto method_cnt_nd = to_integral(methods_nd::methods_count);

  static const char* method_name(methods_1d method) {
    switch (method) {
      case methods_1d::get_global_range:
        return "get_global_range(int) != get_global_range()[int]";
      case methods_1d::get_global_id:
        return "get_global_id(int) != get_global_id()[int]";
      case methods_1d::get_local_range:
        return "get_local_range(int) != get_local_range()[int]";
      case methods_1d::get_local_id:
        return "get_local_id(int) != get_local_id()[int]";
      case methods_1d::get_logical_local_range:
        return "get_logical_local_range(int) != get_logical_local_range()[int]";
      case methods_1d::get_logical_local_id:
        return "get_logical_local_id(int) != get_logical_local_id()[int]";
      case methods_1d::get_physical_local_range:
        return "get_physical_local_range(int) != "
               "get_physrun_1d_checksmethods_ndical_local_range()[int]";
      case methods_1d::get_physical_local_id:
        return "get_physical_local_id(int) != get_physical_local_id()[int]";
      case methods_1d::methods_count:
        return "invalid enum value";
    }
  }

  static const char* method_name(methods_nd method) {
    switch (method) {
      case methods_nd::local_item:
        return "Local item doesn't match logical local item"
               "item.get_local() != item.get_logical_local()";
      case methods_nd::global_range:
        return "Different global ranges"
               "globalItem.get_range() != item.get_global_range()";
      case methods_nd::local_range:
        return "Different local ranges"
               "localItem.get_range() != item.get_local_range()";
      case methods_nd::logical_local_range:
        return "Different logical local ranges"
               "logicalLocalItem.get_range() != item.get_logical_local_range()";
      case methods_nd::physical_local_range:
        return "Different physical local ranges"
               "physicalLocalItem.get_range() != "
               "item.get_physical_local_range()";
      case methods_nd::global_id:
        return "Different global id"
               "globalItem.get_id() != item.get_global_id()";
      case methods_nd::local_id:
        return "Different local id"
               "localItem.get_id() != item.get_local_id()";
      case methods_nd::logical_local_id:
        return "Different logical local id"
               "logicalLocalItem.get_id() != item.get_logical_local_id()";
      case methods_nd::physical_local_id:
        return "Different physical local id"
               "physicalLocalItem.get_id() != item.get_physical_local_id()";
      case methods_nd::methods_count:
        return "invalid enum value";
    }
  }
};

/** @brief Provides core test logic
 */
template <int dims>
class api_tests {
 public:
  /** @brief Entry point for the test
   */
  void operator()();

 private:
  /** @brief Provides device-side checks for a single dimension
   */
  template <int currentDim, typename errorAcc_t>
  static bool run_1d_checks(const sycl::h_item<dims>& item,
                            errorAcc_t errorAcc);

  /** @brief Provides device-side checks applicable for all dimensions
   *  @param ids Variable to store ID values into for later verification on the
   *             host side
   */
  template <typename errorAcc_t>
  static bool run_nd_checks(const sycl::h_item<dims>& item, work_item_ids& ids,
                            errorAcc_t errorAcc);

  /** @brief Counts unique id values and validates value ranges
   */
  template <typename id_descriptor_t>
  static std::vector<size_t> count_ids(const offset_helper& offsets,
                                       const std::vector<work_item_ids>& ids,
                                       const work_item_ids& initialIds,
                                       const work_item_ids& maxIds);

  /** @brief Validates id count values
   */
  template <typename id_descriptor_t>
  static void validate_id_count(const std::vector<size_t>& count,
                                const work_item_ids& expected);
};

template <int dims>
void api_tests<dims>::operator()() {
  auto queue = util::get_cts_object::queue();

  const auto kernelGroupRange = util::get_cts_object::range<dims>::get(3, 4, 2);
  const auto kernelPhysicalLocalRange =
      util::get_cts_object::range<dims>::get(4, 2, 1);
  const auto kernelLogicalLocalRange =
      util::get_cts_object::range<dims>::get(8, 4, 3);

  const size_t numWorkGroups = kernelGroupRange.size();
  const size_t numLogicalPerGroup = kernelLogicalLocalRange.size();
  const size_t numPhysicalPerGroup = kernelPhysicalLocalRange.size();
  const size_t numLogicalWorkItems = numWorkGroups * numLogicalPerGroup;
  const size_t numPhysicalWorkItems = numWorkGroups * numPhysicalPerGroup;
  const size_t numLogicalPerPhysical = numLogicalPerGroup / numPhysicalPerGroup;

  const offset_helper offsets(numLogicalPerGroup, numLogicalWorkItems);

  const work_item_ids initialValue = {43210, 43211, 43212};
  std::vector<int> consistency(offsets.total(), false);
  std::vector<work_item_ids> ids(offsets.total(), initialValue);

  const int errorSize_1d = getter::method_cnt_1d;
  const int errorSize_nd = getter::method_cnt_nd;
  std::vector<int> errorData_1d(errorSize_1d, true);
  std::vector<int> errorData_nd(errorSize_nd, true);
  {
    sycl::range<1> offsetsRange(offsets.total());

    sycl::buffer<int> consistencyBuf(consistency.data(), offsetsRange);
    sycl::buffer<work_item_ids> idsBuf(ids.data(), offsetsRange);
    sycl::buffer<int> errorBuf_1d(errorData_1d.data(), {errorSize_1d});
    sycl::buffer<int> errorBuf_nd(errorData_nd.data(), {errorSize_nd});

    queue.submit([&](sycl::handler& cgh) {
      auto consistency_acc =
          consistencyBuf.get_access<sycl::access_mode::write>(cgh);
      auto id_acc = idsBuf.get_access<sycl::access_mode::write>(cgh);

      auto errorAcc_1d = errorBuf_1d.get_access<sycl::access_mode::write>(cgh);
      auto errorAcc_nd = errorBuf_nd.get_access<sycl::access_mode::write>(cgh);

      cgh.parallel_for_work_group<kernel_common<dims>>(
          kernelGroupRange, kernelPhysicalLocalRange,
          [=](sycl::group<dims> group) {
            group.parallel_for_work_item(
                kernelLogicalLocalRange, [&](sycl::h_item<dims> item) {
                  bool success = true;
                  const size_t offset = offsets.get(group, item);

                  if (offset > offsets.max()) {
                    /** There is no way to store result anyway, so just skip
                     *  the check to preserve the initial values and fail.
                     */
                    return;
                  }

                  success &= run_nd_checks(item, id_acc[offset], errorAcc_nd);

                  if constexpr (dims >= 1) {
                    success &= run_1d_checks<0>(item, errorAcc_1d);
                  }
                  if constexpr (dims >= 2) {
                    success &= run_1d_checks<1>(item, errorAcc_1d);
                  }
                  if constexpr (dims >= 3) {
                    success &= run_1d_checks<2>(item, errorAcc_1d);
                  }

                  consistency_acc[offset] = success;
                });
          });
    });
  }

  // Check api 1d call results
  for (int i = 0; i < errorSize_1d; i++) {
    INFO("Dimensions: " << std::to_string(dims));
    INFO("h_item " << getter::method_name(static_cast<getter::methods_1d>(i))
                   << " tests fails");
    CHECK(errorData_1d[i] != 0);
  }

  // Check api nd call results
  for (int i = 0; i < errorSize_nd; i++) {
    INFO("Dimensions: " << std::to_string(dims));
    INFO("h_item " << getter::method_name(static_cast<getter::methods_nd>(i))
                   << " tests fails");
    CHECK(errorData_nd[i] != 0);
  }

  // Validate consistency
  for (size_t i = 0; i < ids.size(); ++i) {
    const auto& id = ids[i];
    const bool isConsistent = consistency[i];

    INFO("API consistency checks failed or not run for " +
         offsets.to_string<dims>(i));
    CHECK(isConsistent);
  }

  // Count unique id values and validate value ranges
  work_item_ids maxValue;
  maxValue.global = numPhysicalWorkItems - 1;
  maxValue.physical = numPhysicalPerGroup - 1;
  maxValue.logical = numLogicalPerGroup - 1;

  const auto global_count =
      count_ids<global_id>(offsets, ids, initialValue, maxValue);
  const auto physical_count =
      count_ids<physical_id>(offsets, ids, initialValue, maxValue);
  const auto logical_count =
      count_ids<logical_id>(offsets, ids, initialValue, maxValue);

  // Validate unique id values count
  work_item_ids expectedCount;
  expectedCount.global = numLogicalPerPhysical;
  expectedCount.physical = numWorkGroups * numLogicalPerPhysical;
  expectedCount.logical = numWorkGroups;

  validate_id_count<global_id>(global_count, expectedCount);
  validate_id_count<physical_id>(physical_count, expectedCount);
  validate_id_count<logical_id>(logical_count, expectedCount);
}

template <int dims>
template <typename id_descriptor_t>
std::vector<size_t> api_tests<dims>::count_ids(
    const offset_helper& offsets, const std::vector<work_item_ids>& ids,
    const work_item_ids& initialIds, const work_item_ids& maxIds) {
  const size_t max = id_descriptor_t::value(maxIds);
  const size_t initial = id_descriptor_t::value(initialIds);

  std::vector<size_t> count(max + 1, 0);

  for (size_t i = 0; i < ids.size(); ++i) {
    const size_t& value = id_descriptor_t::value(ids[i]);

    if (value == initial) {
      std::string message;
      message += "No " + id_descriptor_t::description() + " stored for ";
      message += offsets.to_string<dims>(i);
      FAIL(message);
    } else if (value > max) {
      std::string message;
      message += "Too big " + id_descriptor_t::description() + " value ";
      message += std::to_string(value) + " for ";
      message += offsets.to_string<dims>(i);
      FAIL(message);
    } else {
      count[value] += 1;
    }
  }
  return count;
}

template <int dims>
template <typename id_descriptor_t>
void api_tests<dims>::validate_id_count(const std::vector<size_t>& count,
                                        const work_item_ids& expectedValues) {
  for (size_t i = 0; i < count.size(); ++i) {
    const auto& expected = id_descriptor_t::value(expectedValues);
    if (count[i] != expected) {
      std::string message("Unexpected number of occurences: ");
      message += std::to_string(count[i]);
      message += " for " + id_descriptor_t::description();
      message += ", value " + std::to_string(i);
      message += ", dimensions: " + std::to_string(dims);
      message += "; expected: " + std::to_string(expected);
      FAIL(message);
    }
  }
}

template <int dims>
template <int currentDim, typename errorAcc_t>
bool api_tests<dims>::run_1d_checks(const sycl::h_item<dims>& item,
                                    errorAcc_t errorAcc) {
  {
    auto value = item.get_global_range(currentDim);
    auto expected = item.get_global_range()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_global_range)] =
        (value == expected);
  }
  {
    auto value = item.get_global_id(currentDim);
    auto expected = item.get_global_id()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_global_id)] =
        (value == expected);
  }
  {
    auto value = item.get_local_range(currentDim);
    auto expected = item.get_local_range()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_local_range)] =
        (value == expected);
  }
  {
    auto value = item.get_local_id(currentDim);
    auto expected = item.get_local_id()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_local_id)] =
        (value == expected);
  }
  {
    auto value = item.get_logical_local_range(currentDim);
    auto expected = item.get_logical_local_range()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_logical_local_range)] =
        (value == expected);
  }
  {
    auto value = item.get_logical_local_id(currentDim);
    auto expected = item.get_logical_local_id()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_logical_local_id)] =
        (value == expected);
  }
  {
    auto value = item.get_physical_local_range(currentDim);
    auto expected = item.get_physical_local_range()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_physical_local_range)] =
        (value == expected);
  }
  {
    auto value = item.get_physical_local_id(currentDim);
    auto expected = item.get_physical_local_id()[currentDim];
    errorAcc[to_integral(getter::methods_1d::get_physical_local_id)] =
        (value == expected);
  }
  return std::all_of(errorAcc.begin(), errorAcc.end(),
                     [](int val) { return val; });
}

template <int dims>
template <typename errorAcc_t>
bool api_tests<dims>::run_nd_checks(const sycl::h_item<dims>& item,
                                    work_item_ids& ids, errorAcc_t errorAcc) {
  static constexpr bool with_offset = false;

  // Get items
  sycl::item<dims, with_offset> globalItem = item.get_global();
  sycl::item<dims, with_offset> localItem = item.get_local();
  sycl::item<dims, with_offset> logicalLocalItem = item.get_logical_local();
  sycl::item<dims, with_offset> physicalLocalItem = item.get_physical_local();

  // Check items
  errorAcc[to_integral(getter::methods_nd::local_item)] =
      (localItem == logicalLocalItem);

  // Store item linear IDs to verify all are present
  ids.global = globalItem.get_linear_id();
  ids.physical = physicalLocalItem.get_linear_id();
  ids.logical = logicalLocalItem.get_linear_id();

  // Get ranges
  using range_t = sycl::range<dims>;
  range_t globalRange = item.get_global_range();
  range_t localRange = item.get_local_range();
  range_t logicalLocalRange = item.get_logical_local_range();
  range_t physicalLocalRange = item.get_physical_local_range();

  // Check ranges
  errorAcc[to_integral(getter::methods_nd::global_range)] =
      (globalItem.get_range() == globalRange);
  errorAcc[to_integral(getter::methods_nd::local_range)] =
      (localItem.get_range() == localRange);
  errorAcc[to_integral(getter::methods_nd::logical_local_range)] =
      (logicalLocalItem.get_range() == logicalLocalRange);
  errorAcc[to_integral(getter::methods_nd::physical_local_range)] =
      (physicalLocalItem.get_range() == physicalLocalRange);

  // Get IDs
  using id_t = sycl::id<dims>;
  id_t globalId = item.get_global_id();
  id_t localId = item.get_local_id();
  id_t logicalLocalId = item.get_logical_local_id();
  id_t physicalLocalId = item.get_physical_local_id();

  // Check IDs
  errorAcc[to_integral(getter::methods_nd::global_id)] =
      (globalItem.get_id() == globalId);
  errorAcc[to_integral(getter::methods_nd::local_id)] =
      (localItem.get_id() == localId);
  errorAcc[to_integral(getter::methods_nd::logical_local_id)] =
      (logicalLocalItem.get_id() == logicalLocalId);
  errorAcc[to_integral(getter::methods_nd::physical_local_id)] =
      (physicalLocalItem.get_id() == physicalLocalId);

  return std::all_of(errorAcc.begin(), errorAcc.end(),
                     [](int val) { return val; });
}

TEST_CASE("h_item_1d API", "[h_item]") { api_tests<1>{}(); }

TEST_CASE("h_item_2d API", "[h_item]") { api_tests<2>{}(); }

TEST_CASE("h_item_3d API", "[h_item]") { api_tests<3>{}(); }

}  // namespace h_item_api
