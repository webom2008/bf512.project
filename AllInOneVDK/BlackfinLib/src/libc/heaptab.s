.section constdata;
.file_attr libName=libc;
.file_attr libFunc=_heap_table;
.file_attr FuncName=_heap_table;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

/* Ensure global data is in shared memory for multicore programs. */
.file_attr ThreadPerCoreSharing=MustShare;

.extern ldf_heap_space;
.extern ldf_heap_length;

.align 4;
_heap_table:
	// struct { void *base; size_t length; long int userid; }

	// Default heap must come first, and have userID==0.
	.var = ldf_heap_space;
	.var = ldf_heap_length;
	.var = 0;

	// Any other heaps go in here

	// Table terminator
	.var = 0, 0, 0;
._heap_table.end:

.global _heap_table;

