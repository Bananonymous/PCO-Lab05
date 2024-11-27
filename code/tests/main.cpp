#include <gtest/gtest.h>

#include "quicksort.h"
#include "utils.h"

/**
 * @brief test Generates a random sequence of specified size and sorts it with Quicksort using N threads.
 * @param nbThreads number of threads to use to sort the sequence
 * @param size of the sequence to sort
 * @param seed to use for the random generation of the sequence
 */
void test(int nbThreads, int size, int seed) {
    Quicksort<int> sorter(nbThreads);
    std::vector<int> array = generateSequence(size, seed);
    sorter.sort(array);
    EXPECT_FALSE(array.empty());  // check that the result is not empty
    EXPECT_TRUE(isSorted(array)); // check that result is sorted
}


// Test cases for Quicksort
TEST(SortingTest, EmptyArray) {
    Quicksort<int> sorter(2);
    std::vector<int> array; // Empty array
    sorter.sort(array);
    EXPECT_TRUE(array.empty());
}

TEST(SortingTest, SingleElement) {
    Quicksort<int> sorter(2);
    std::vector<int> array = {42}; // Array with one element
    sorter.sort(array);
    EXPECT_EQ(array.size(), 1);
    EXPECT_EQ(array[0], 42);
}

TEST(SortingTest, IdenticalElements) {
    Quicksort<int> sorter(2);
    std::vector<int> array = {5, 5, 5, 5, 5}; // Array with identical elements
    sorter.sort(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_TRUE(isSorted(array));
}

TEST(SortingTest, SmallArray) {
    test(2, 10, 1); // Small array with 10 elements
}

TEST(SortingTest, LargeArraySingleThread) {
    test(1, 100000, 42); // Large array with 100,000 elements, single-threaded
}

TEST(SortingTest, LargeArrayMultiThread) {
    test(4, 100000, 42); // Large array with 100,000 elements, multi-threaded
}

TEST(SortingTest, BigBoyArraySingleThread) {
    test(1, 10000000, 42); // Large array with 10,000,000 elements, single-threaded
}

TEST(SortingTest, BigBoyArrayMultiThread) {
    test(4, 10000000, 42); // Large array with 10,000,000 elements, multi-threaded
}

TEST(SortingTest, RandomSeed) {
    test(4, 100, time(nullptr)); // Random seed for varied data
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
