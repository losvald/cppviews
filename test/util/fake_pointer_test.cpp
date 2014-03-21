#include "../src/util/fake_pointer.hpp"
#include "test.hpp"

TEST(FakePointerTest, String) {
  using namespace std;

  FakePointer<string> uninit;

  std::string foo_string("foo");
  FakePointer<string> foo(foo_string);
  EXPECT_EQ("foo", *foo);

  FakePointer<string> aaa(3, 'a');
  EXPECT_EQ("aaa", *aaa);

  FakePointer<string> bar("bar");
  EXPECT_EQ("bar", *bar);

  foo->append("bar");
  EXPECT_STREQ("foobar", foo->c_str());

  FakePointer<string> foobar(foo);
  const char* foobar_cstr = foobar->c_str();
  EXPECT_STREQ("foobar", foobar_cstr);
  // EXPECT_NE(foo->c_str(), foobar_cstr);

  FakePointer<string> foobar_moved(std::move(foobar));
  EXPECT_EQ("foobar", *foobar_moved);
  EXPECT_EQ(foobar_cstr, foobar_moved->c_str());

  foobar->clear();
  EXPECT_TRUE(foobar->empty());

  auto foobar_copied = foobar_moved;
  EXPECT_EQ("foobar", *foobar_moved);
  EXPECT_EQ("foobar", *foobar_copied);
}
