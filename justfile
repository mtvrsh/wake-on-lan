build: build-c build-go build-rust

clean: clean-c clean-go clean-rust

build-c:
    meson setup c/build c
    ninja -C c/build

clean-c:
    rm -rf c/build

build-go:
    go build -C go-wol

clean-go:
    rm -f go-wol/go-wol

build-rust:
    cd rust && cargo build

clean-rust:
    cd rust && cargo clean
