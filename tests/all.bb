!modify "bb://.cache/"
!modify "vfs://"

test("Errors")     {!include "tests/errors"}
test("Default")    {!include "tests/default"}
test("Strings")    {!include "tests/strings"}
test("String add") {!include "tests/concat"}
test("List")       {!include "tests/list"}
test("Range")      {!include "tests/range"}
test("Iteration")  {!include "tests/iter"}
test("Map")        {!include "tests/map"}
test("Vector")     {!include "tests/vector"}
test("Gather")     {!include "tests/gather"}
test("Overload")   {!include "tests/overload"}
test("Func call")  {!include "tests/calls"}
test("Sideerrors") {!include "tests/sideerrors"}
test("Closure")    {!include "tests/closure"}
test("Clear")      {!include "tests/clear"}
test("Flat")       {!include "tests/flat"}
test("Move ")      {!include "tests/move"}
test("Collection") {!include "tests/collection"}
test("Stringutil") {!include "tests/string"}
test("Atomicity")  {!include "tests/atomicity"}
test("No deadlock"){!include "tests/nodeadlock"}
test("VFS")        {!include "tests/vfs"}
test("Database")   {!include "tests/database"}
test("RAII")       {!include "tests/raii"}
