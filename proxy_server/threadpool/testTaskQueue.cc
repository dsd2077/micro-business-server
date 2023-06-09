#include "TaskQueue.hpp"
#include <thread>
#include <gtest/gtest.h>
using namespace wd;

// TEST(TaskQueueTest, EmptyQueueReturnsTrue) {
//   // Arrange
//   TaskQueue<int> queue(10);

//   // Assert
//   EXPECT_TRUE(queue.empty());
// }


// TEST(TaskQueueTest, FullQueueReturnsTrue) {
//   // Arrange
//   TaskQueue<int> queue(1);
//   queue.push(1);

//   // Assert
//   EXPECT_TRUE(queue.full());
// }

// TEST(TaskQueueTest, PushAndPopSingleElement) {
//   // Arrange
//   TaskQueue<int> queue(10);
//   int expectedValue = 42;

//   // Act
//   queue.push(expectedValue);
//   int actualValue = queue.pop();

//   // Assert
//   EXPECT_EQ(expectedValue, actualValue);
// }

// TEST(TaskQueueTest, PopOnEmptyQueueBlocks) {
//   // Arrange
//   TaskQueue<int> queue(10);

//   // Act
//   std::thread t([&queue]() {
//     queue.pop();
//   });
//   std::this_thread::sleep_for(std::chrono::milliseconds(100));
//   queue.wakeup();
//   t.join();

//   // Assert
//   EXPECT_TRUE(true);
// }

// TEST(TaskQueueTest, TestPushPop) {
//     const int queueSize = 10;
//     TaskQueue<int> q(queueSize);

//     // Push and pop elements
//     for (int i = 0; i < queueSize; ++i) {
//         EXPECT_TRUE(q.empty());
//         EXPECT_FALSE(q.full());
//         q.push(i);
//         EXPECT_FALSE(q.empty());
//         EXPECT_EQ(q.pop(), i);
//     }

//     // Queue should be empty now
//     EXPECT_TRUE(q.empty());
//     EXPECT_FALSE(q.full());
// }

TEST(TaskQueueTest, TestBlocking) {
    const int queueSize = 1;
    TaskQueue<int> q(queueSize);

    std::thread t([&]() {
        // This push will block since the queue is full
        q.push(1);
        // Wakeup the consumer thread
        // q.wakeup();
    });

    // Wait for the producer thread to block
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // The queue should be full now
    // EXPECT_FALSE(q.empty());
    // EXPECT_TRUE(q.full());

    // Pop the element from the queue
    EXPECT_EQ(q.pop(), 1);

    // Consumer thread should have already resumed
    t.join();

    // Queue should be empty again
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}