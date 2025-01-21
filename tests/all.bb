!modify "bb://.cache/"
!modify "vfs://"

test("String add") {!include "tests/concat"}
test("List")       {!include "tests/list"}
test("Range")      {!include "tests/range"}
test("Iteration")  {!include "tests/iter"}
test("Vector")     {!include "tests/vector"}
test("Overload")   {!include "tests/overload"}
test("Closure")    {!include "tests/closure"}
test("Clear")      {!include "tests/clear"}
test("Move ")      {!include "tests/move"}
test("Collection") {!include "tests/collection"}
//test("Stringutil") {!include "tests/string"}
test("Atomicity")  {!include "tests/atomicity"}
test("No deadlock"){!include "tests/nodeadlock"}
test("VFS")        {!include "tests/vfs"}
test("Database")   {!include "tests/database"}
