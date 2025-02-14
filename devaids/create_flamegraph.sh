static_eraser/static_eraser & pid=$! && \
sudo /usr/lib/linux-tools/5.15.0-131-generic/perf record -F 99 --call-graph dwarf -p $pid -g; \
sudo /usr/lib/linux-tools/5.15.0-131-generic/perf script > out.perf; \
sudo FlameGraph/stackcollapse-perf.pl out.perf > out.folded; \
sudo FlameGraph/flamegraph.pl out.folded > graph.svg; \
sudo rm out.perf; sudo rm out.folded; sudo rm perf.data;