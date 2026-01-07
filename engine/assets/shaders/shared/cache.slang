module cache;

/* Cache information. (e.g. capacity) */
public struct CacheInfo {
    public uint capacity;
};

/* Cache entry data. */
public struct CacheEntry {
    public uint64_t key;
    public uint occupancy;
    public uint last_used;
};

namespace cache {

} // cache
