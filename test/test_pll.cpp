#include "gtest/gtest.h"
#include "pll.h"

TEST(PLLTest, ReachabilityTest) {
    Graph g;
    g.addEdge(1, 2);
    g.addEdge(2, 3);

    PLL pll(g);
    pll.buildPLLLabels();

    EXPECT_TRUE(pll.reachabilityQuery(1, 3));
    EXPECT_FALSE(pll.reachabilityQuery(3, 1));
}
