#include "pch.h"

#include "..\requirements\requirements.hpp"

enum NiceGuys
{
    Kyle,
    John,
    Harry,
    Jack,
    Joe
};
class RequirementsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        req1.add(Kyle, Jack);   // Kyle requires Jack
        req1.add(Jack, John);   // Jack requires John
        req1.add(Joe, John);    // Joe requires John
        req2.add(Harry, Joe);   // Harry requires Joe
        req2.add(Joe, Harry);   // Joe requires Harry (only allowed if reflexive)
    }

    // void TearDown() override {}

    Requirements<NiceGuys> req0{ false };
    Requirements<NiceGuys> req1{ false };
    Requirements<NiceGuys> req2{ true };
};

using RequirementsDeathTest = RequirementsTest;

TEST_F(RequirementsTest, Initialization)
{
    EXPECT_FALSE(req0.reflexive());
    EXPECT_TRUE(req0.empty());
    EXPECT_EQ(req0.size(), 0);
    EXPECT_FALSE(req1.reflexive());
    EXPECT_FALSE(req1.empty());
    EXPECT_EQ(req1.size(), 3);
    EXPECT_TRUE(req2.reflexive());
    EXPECT_FALSE(req2.empty());
    EXPECT_EQ(req2.size(), 2);
}

TEST_F(RequirementsDeathTest, Assertion_If_Non_Reflexive)
{
    EXPECT_DEATH(req1.add(Jack, Kyle), "");     // only allowed if reflexive but req1 is not reflexive
}

TEST_F(RequirementsTest, Exists)
{
    EXPECT_TRUE(req1.exists(Kyle, Jack) && req1.exists(Jack, John) && req1.exists(Joe, John));
    EXPECT_TRUE(req2.exists(Harry, Joe) && req2.exists(Joe, Harry));
}

TEST_F(RequirementsTest, Has_Requirements)
{
    EXPECT_TRUE(req1.has_requirements(Kyle));
    EXPECT_TRUE(req1.has_requirements(Jack));
    EXPECT_TRUE(req1.has_requirements(Joe));
    EXPECT_FALSE(req1.has_requirements(John));
    EXPECT_FALSE(req1.has_requirements(Harry));
    EXPECT_TRUE(req2.has_requirements(Harry));
    EXPECT_TRUE(req2.has_requirements(Joe));
    EXPECT_FALSE(req2.has_requirements(Kyle));
    EXPECT_FALSE(req2.has_requirements(Jack));
    EXPECT_FALSE(req2.has_requirements(John));
}

TEST_F(RequirementsTest, Has_Dependents)
{
    EXPECT_TRUE(req1.has_dependents(Jack));
    EXPECT_TRUE(req1.has_dependents(John));
    EXPECT_FALSE(req1.has_dependents(Kyle));
    EXPECT_FALSE(req1.has_dependents(Harry));
    EXPECT_FALSE(req1.has_dependents(Joe));
    EXPECT_TRUE(req2.has_dependents(Harry));
    EXPECT_TRUE(req2.has_dependents(Joe));
    EXPECT_FALSE(req2.has_dependents(Kyle));
    EXPECT_FALSE(req2.has_dependents(Jack));
    EXPECT_FALSE(req2.has_dependents(John));
}

TEST_F(RequirementsTest, Dependents)
{
    auto deps = req1.dependents(John);
    ASSERT_EQ(deps.size(), 2);
    EXPECT_TRUE(deps[0] == Jack || deps[0] == Joe);
    if (deps[0] == Jack)
        EXPECT_EQ(deps[1], Joe);
    else
        EXPECT_EQ(deps[1], Jack);
}

TEST_F(RequirementsTest, All_Requirements)
{
    auto dep_paths = req1.all_requirements(true);
    EXPECT_EQ(dep_paths.size(), 2);
    for (auto path : dep_paths)
        EXPECT_EQ(path.back(), John);
}

TEST_F(RequirementsTest, All_Dependencies)
{
    auto req_paths = req1.all_dependencies(true);
    EXPECT_EQ(req_paths.size(), 2);
    for (auto path : req_paths)
        EXPECT_TRUE(path.back() == Joe || path.back() == Kyle);
}

TEST_F(RequirementsTest, Requires)
{
    EXPECT_TRUE(req1.requires(Kyle, John));
    EXPECT_FALSE(req1.requires(Jack, Joe));
    EXPECT_TRUE(req2.requires(Harry, Joe));
    EXPECT_TRUE(req2.requires(Joe, Harry));
}

TEST_F(RequirementsTest, Depends)
{
    EXPECT_TRUE(req1.depends(John, Kyle));
    EXPECT_FALSE(req1.depends(Jack, Joe));
    EXPECT_TRUE(req2.depends(Harry, Joe));
    EXPECT_TRUE(req2.depends(Joe, Harry));
}

TEST_F(RequirementsTest, Remove_All)
{
    req1.remove_all(Jack);
    EXPECT_FALSE(req1.has_requirements(Kyle));
    EXPECT_FALSE(req1.exists(Jack, John));
    EXPECT_EQ(req1.size(), 1);
}

TEST_F(RequirementsTest, Clear)
{
    req1.clear();
    req2.clear();
    EXPECT_EQ(req1.size(), 0);
    EXPECT_TRUE(req1.empty());
    EXPECT_EQ(req2.size(), 0);
    EXPECT_TRUE(req2.empty());
}
