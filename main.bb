test = new {
    x = new{y=0;z=1}
    y = new{x=0;z=2}

    x.y = y;
    y.x = x;

    w = new{x=x;y=y}
    x = 0;
    y = 0;

    std::print(w.x.y.z);
}

test.w.x.y = 0;

std::print(test.w);