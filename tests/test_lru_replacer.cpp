#include "../include/lru_replacer.h"
#include <cassert>
#include <cstdio>

int main() {
    LRUReplacer replacer(5);

    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(3);  // evictable order, oldest to newest: 1, 2, 3

    frame_id_t victim;
    assert(replacer.Victim(&victim) && victim == 1);
    printf("[PASS] First victim is %d (expected 1 — oldest unpinned)\n", victim);

    assert(replacer.Victim(&victim) && victim == 2);
    printf("[PASS] Second victim is %d (expected 2)\n", victim);

    replacer.Unpin(4);
    replacer.Pin(3);  // remove 3 from candidacy entirely

    assert(replacer.Victim(&victim) && victim == 4);
    printf("[PASS] After pinning 3, victim is %d (expected 4, since 3 is excluded)\n", victim);

    assert(!replacer.Victim(&victim));
    printf("[PASS] Replacer correctly reports empty once exhausted\n");

    printf("\nAll LRUReplacer tests passed!\n");
    return 0;
}
