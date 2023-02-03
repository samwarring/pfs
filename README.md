# PFS
PFS (Polymorphic File System) is a C++ header-only library that wraps `std::filesystem` to facilitate dependency injection. It achieves this with an abstract interface and implementations of the interface for production and test environments.

## Motivation
Unit-testing C++ software that relys on the filesystem is undesireable because it is subject to the wider environment and may not be repeatable on different machines. A common solution to this problem is to use dependency-injection. We introduce a filesystem interface with a mock implementation for unit-testing. However, this has its own shortcoming of not accurately capturing the nuanced behavior of your filesystem.

PFS is a complete solution to your dependency-injected filesystem. Not only does it provide the interface and production implementaiton, but it also provides an in-memory "fake filesystem" that emulates real fileystem behavior. The interface is designed to match the `std::filesystem` library as closely as possible.

## Example

```cpp
// ----------------------------------------------
// Widget.hpp
// ----------------------------------------------

#include <pfs/filesystem.hpp>

class Widget {
    pfs::filesystem* fs;

    Widget(pfs::filesystem& fs) : fs(fs) {}

    void DoSomething() {
        pfs->create_directory("widget_workspace");
        std::unique_ptr<std::ostream> f = pfs->open_file("widget_workspace/log.txt");
        f << "The answer is: " << 42;
    }
};

// ----------------------------------------------
// Main.cpp
// ----------------------------------------------

#include <pfs/std_filesystem.hpp>
#include <Widget.hpp>

int main() {
    pfs::std_filesystem fs;
    Widget w(fs);
    w.DoSomething();
}

// ----------------------------------------------
// WidgetTest.cpp
// ----------------------------------------------

#include <pfs/fake_filesystem.hpp>
#include <Widget.hpp>
#include <TestFramework.hpp>

TEST_CASE("Testing the widget")
{
    pfs::fake_filesystem fs;
    Widget w(fs);
    w.DoSomething();
    ASSERT(fs.is_regular_file("widget_workspace/log.txt"));
}
```