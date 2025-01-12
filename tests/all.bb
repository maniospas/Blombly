!modify ".cache/"

test("List")       {!include "tests/list"}
test("Range")      {!include "tests/range"}
test("Iteration")  {!include "tests/iter"}
test("Closure")    {!include "tests/closure"}
test("Collection") {!include "tests/collection"}
test("String")     {!include "tests/string"}
test("Atomicity")  {!include "tests/atomicity"}
test("No deadlock"){!include "tests/nodeadlock"}
test("Database")   {!include "tests/database"}
