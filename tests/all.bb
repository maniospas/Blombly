!modify ".cache/"

test("String add") {!include "tests/concat"}
test("List")       {!include "tests/list"}
test("Range")      {!include "tests/range"}
test("Iteration")  {!include "tests/iter"}
test("Overload")   {!include "tests/overload"}
test("Closure")    {!include "tests/closure"}
test("Clear")      {!include "tests/clear"}
test("Move ")      {!include "tests/move"}
test("Collection") {!include "tests/collection"}
test("String")     {!include "tests/string"}
test("Atomicity")  {!include "tests/atomicity"}
test("No deadlock"){!include "tests/nodeadlock"}
test("Database")   {!include "tests/database"}
