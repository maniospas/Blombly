!comptime(try{
    received_hash = bb.os.transfer("libs/html.bb", "https://raw.githubusercontent.com/maniospas/Blombly/refs/heads/main/libs/html.bb");
    assert "d369d06ae1e60992b12f1417e60817d5" == received_hash;
    return true;
});
