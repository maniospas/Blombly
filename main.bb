method = new {
    final test1 = {
        print("test1");
    }
    final test2 = {
        test1:
        print("test2");
    }
    \call = test2;
}

method();