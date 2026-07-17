#include "../include/bplus_tree.h"
#include "../include/buffer_pool.h"
#include "../include/disk_manager.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <random>
#include <set>

page_id_t MakeEmptyRoot(DiskManager& dm, BufferPool& bpm) {
    page_id_t root_id = dm.AllocatePage();
    Page* page = bpm.FetchPage(root_id);
    BPlusTreeNode node(page);
    node.SetIsLeaf(true);
    node.SetNumKeys(0);
    node.SetParentId(0);
    node.SetNextLeafId(0);
    bpm.UnpinPage(root_id, true);
    return root_id;
}

void TestEmptyTreeFirstInsert() {
    DiskManager dm("test_stress_empty.db");
    BufferPool bpm(20, &dm);
    page_id_t root_id = MakeEmptyRoot(dm, bpm);
    BPlusTree tree(&bpm, &dm, &root_id);

    value_t v;
    assert(!tree.Search(1, &v));
    tree.Insert(1, 111);
    assert(tree.Search(1, &v) && v == 111);
    printf("[PASS] First insert into empty tree works correctly\n");
}

void TestDuplicateKeyPolicy() {
    DiskManager dm("test_stress_dup.db");
    BufferPool bpm(20, &dm);
    page_id_t root_id = MakeEmptyRoot(dm, bpm);
    BPlusTree tree(&bpm, &dm, &root_id);

    tree.Insert(10, 100);
    tree.Insert(10, 999);  // duplicate: should be rejected

    value_t v;
    assert(tree.Search(10, &v));
    assert(v == 100);
    printf("[PASS] Duplicate key insert rejected, original value (100) preserved\n");
}

void TestSequentialAscendingStress() {
    DiskManager dm("test_stress_seq.db");
    BufferPool bpm(50, &dm);
    page_id_t root_id = MakeEmptyRoot(dm, bpm);
    BPlusTree tree(&bpm, &dm, &root_id);

    const int N = 200;
    for (int k = 1; k <= N; k++) {
        tree.Insert(k, k * 10);
    }
    printf("[PASS] Sequential ascending insert of %d keys completed\n", N);

    value_t v;
    for (int k = 1; k <= N; k++) {
        assert(tree.Search(k, &v) && v == k * 10);
    }
    printf("[PASS] All %d sequential keys found with correct values\n", N);

    assert(!tree.Search(0, &v));
    assert(!tree.Search(N + 1, &v));
    printf("[PASS] Out-of-range keys correctly not found\n");

    auto results = tree.RangeScan(1, N);
    assert((int)results.size() == N);
    for (int i = 0; i < N; i++) {
        assert(results[i].first == i + 1);
        assert(results[i].second == (i + 1) * 10);
    }
    printf("[PASS] RangeScan(1,%d) returns all keys sorted, no gaps/dupes\n", N);
}

void TestRandomStress10000() {
    DiskManager dm("test_stress_random.db");
    BufferPool bpm(50, &dm);
    page_id_t root_id = MakeEmptyRoot(dm, bpm);
    BPlusTree tree(&bpm, &dm, &root_id);

    const int N = 10000;
    std::vector<int> keys(N);
    for (int i = 0; i < N; i++) keys[i] = i + 1;

    std::mt19937 rng(42);  // fixed seed for reproducibility
    std::shuffle(keys.begin(), keys.end(), rng);

    for (int k : keys) {
        tree.Insert(k, k * 10);
    }
    printf("[PASS] Inserted %d keys in random order without crashing\n", N);

    value_t v;
    for (int k = 1; k <= N; k++) {
        assert(tree.Search(k, &v) && v == k * 10);
    }
    printf("[PASS] All %d inserted keys found with correct values\n", N);

    std::vector<int> never_inserted = {-1, 0, N + 1, N + 100, N + 9999};
    for (int k : never_inserted) {
        assert(!tree.Search(k, &v));
    }
    printf("[PASS] Never-inserted keys correctly report not found\n");

    auto results = tree.RangeScan(1, N);
    assert((int)results.size() == N);
    std::set<int> seen;
    for (int i = 0; i < N; i++) {
        assert(results[i].first == i + 1);
        assert(results[i].second == (i + 1) * 10);
        assert(seen.insert(results[i].first).second);  // no duplicates
    }
    printf("[PASS] RangeScan(1,%d) returns all %d keys sorted, no gaps/dupes\n", N, N);

    // Partial range scan sanity check.
    auto partial = tree.RangeScan(4000, 4010);
    assert(partial.size() == 11);
    for (size_t i = 0; i < partial.size(); i++) {
        assert(partial[i].first == 4000 + (int)i);
    }
    printf("[PASS] Partial RangeScan(4000,4010) returns correct 11-key slice\n");
}

int main() {
    TestEmptyTreeFirstInsert();
    TestDuplicateKeyPolicy();
    TestSequentialAscendingStress();
    TestRandomStress10000();
    printf("\nAll Day 5 stress and edge-case tests passed!\n");
    return 0;
}
