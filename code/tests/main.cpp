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
    EXPECT_FALSE(array.empty());
    EXPECT_TRUE(isSorted(array));
}


// // Test cases for Quicksort
TEST(SortingTest, EmptyArray) {
    Quicksort<int> sorter(2);
    std::vector<int> array(0);
    sorter.sort(array);
    EXPECT_TRUE(array.empty());
}

TEST(SortingTest, SingleElement) {
    Quicksort<int> sorter(2);
    std::vector<int> array = {42};
    sorter.sort(array);
    EXPECT_EQ(array.size(), 1);
    EXPECT_EQ(array[0], 42);
}

TEST(SortingTest, IdenticalElements) {
    Quicksort<int> sorter(2);
    std::vector<int> array = {5, 5, 5, 5, 5};
    sorter.sort(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_TRUE(isSorted(array));
}

TEST(SortingTest, SortedArray) {
    Quicksort<int> sorter(2);
    std::vector<int> array = {1, 2, 3, 4, 5};
    sorter.sort(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_TRUE(isSorted(array));
}


TEST(SortingTest, SmallArraySingleThread) {
    test(1, 10, 1);
}

TEST(SortingTest, SmallArrayMultiThread) {
    test(2, 10, 1);
}

TEST(SortingTest, MediumArraySingleThread) {
    test(1, 50, 1);
}

TEST(SortingTest, MediumArrayMultiThread) {
    test(2, 50, 1);
}

TEST(SortingTest, LargeArraySingleThread) {
    test(1, 100000, 42);
}

TEST(SortingTest, LargeArrayMultiThread) {
    test(2, 100000, 42);
}

TEST(SortingTest, LargerArraySingleThread) {
    test(1, 10000000, 42);
}

TEST(SortingTest, LargerArrayMultiThread) {
    test(5, 10000000, 42);
}

TEST(SortingTest, RandomSeed) {
    test(4, 1000, time(nullptr)); // Random seed for varied data
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
