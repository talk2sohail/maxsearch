package main

import "core:fmt"
import "core:slice"
import "core:strings"
import "core:sync"
import "core:sys/posix"
import "core:thread"

SearchEngine :: struct {
	// Threading
	thread:        ^thread.Thread,
	mutex:         sync.Mutex,
	cond:          sync.Cond,
	is_running:    bool,

	// Shared State
	pending_query: string,
	has_new_query: bool,

	// Results (Shared)
	results:       [dynamic]string,
}

// Global engine context for the worker procedure
// (Odin thread procs usually take a user_data pointer, but let's be explicit)

se_init :: proc(se: ^SearchEngine) {
	se.results = make([dynamic]string)
	se.is_running = true
	se.pending_query = ""
	se.has_new_query = false

	// sync.Mutex and sync.Cond are zero-initialized

	se.thread = thread.create_and_start_with_data(se, se_worker_proc)
}

se_destroy :: proc(se: ^SearchEngine) {
	// Signal thread to stop
	sync.mutex_lock(&se.mutex)
	se.is_running = false
	se.has_new_query = true // Wake it up
	sync.cond_signal(&se.cond)
	sync.mutex_unlock(&se.mutex)

	thread.join(se.thread)
	thread.destroy(se.thread)

	// Cleanup results
	for res in se.results {
		delete(res)
	}
	delete(se.results)

	if len(se.pending_query) > 0 {
		delete(se.pending_query)
	}
}
// Helper: Custom sort comparator
compare_results :: proc(i, j: string) -> bool {
	a := i
	b := j

	a_is_app := strings.has_suffix(a, ".app")
	b_is_app := strings.has_suffix(b, ".app")

	if a_is_app != b_is_app {
		return a_is_app
	}

	if len(a) != len(b) {
		return len(a) < len(b)
	}

	return a < b
}

// UI calls this. NON-BLOCKING.
se_search :: proc(se: ^SearchEngine, query: string) {
	sync.mutex_lock(&se.mutex)
	defer sync.mutex_unlock(&se.mutex)

	// Update pending query
	if len(se.pending_query) > 0 {
		delete(se.pending_query)
	}
	se.pending_query = strings.clone(query)
	se.has_new_query = true

	sync.cond_signal(&se.cond)
}

// UI calls this to draw. Returns a slice of the current results.
// Note: This returns the internal slice. The caller must NOT free the strings inside,
// and should treat it as read-only/volatile if iterating without lock.
// For perfect safety, we should return a copy or hold the lock while drawing.
// Given Raylib's single-threaded draw loop, we can just Copy the results to a temp buffer
// or lock briefly to clone.
//
// Simple approach: Return a copy of the results allocated with context.temp_allocator
se_get_results :: proc(se: ^SearchEngine) -> []string {
	sync.mutex_lock(&se.mutex)
	defer sync.mutex_unlock(&se.mutex)

	// Clone pointers to a new slice, but keep the strings shared?
	// No, if the worker updates 'results', it might delete the strings.
	// So we must clone the strings or ensure the worker doesn't double-free while we read.

	// Safer: Clone everything to temp_allocator
	res_copy := make([dynamic]string, context.temp_allocator)
	for s in se.results {
		append(&res_copy, strings.clone(s, context.temp_allocator))
	}

	return res_copy[:]
}

se_worker_proc :: proc(data: rawptr) {
	se := cast(^SearchEngine)data

	local_query := ""

	for {
		// Wait for work
		sync.mutex_lock(&se.mutex)
		for se.is_running && !se.has_new_query {
			sync.cond_wait(&se.cond, &se.mutex)
		}

		if !se.is_running {
			sync.mutex_unlock(&se.mutex)
			break
		}

		// Get work
		if len(local_query) > 0 {
			delete(local_query)
		}
		local_query = strings.clone(se.pending_query)
		se.has_new_query = false
		sync.mutex_unlock(&se.mutex)

		// PERFORM SEARCH (Blocking part, now off main thread)
		new_results := perform_mdfind(local_query)

		// Update Shared State
		sync.mutex_lock(&se.mutex)

		// Cleanup old results
		for res in se.results {
			delete(res)
		}
		clear(&se.results)

		// Move new results in
		for res in new_results {
			append(&se.results, res)
		}
		delete(new_results) // Delete the dynamic array wrapper, not the strings inside (they moved)

		sync.mutex_unlock(&se.mutex)
	}

	if len(local_query) > 0 {
		delete(local_query)
	}
}

perform_mdfind :: proc(query: string) -> [dynamic]string {
	results := make([dynamic]string)
	if len(query) == 0 do return results

	safe_query := strings.clone(query)
	defer delete(safe_query)

	cmd_str := fmt.tprintf(
		"mdfind 'kMDItemDisplayName == \"%s*\"c || kMDItemDisplayName == \"* %s*\"c' | head -n 50",
		safe_query,
		safe_query,
	)

	cmd_cstr := strings.clone_to_cstring(cmd_str, context.temp_allocator)
	mode_cstr := strings.clone_to_cstring("r", context.temp_allocator)

	fp := posix.popen(cmd_cstr, mode_cstr)
	if fp == nil {
		return results
	}
	defer posix.pclose(fp)

	buf: [1024]byte
	for posix.fgets(&buf[0], 1024, fp) != nil {
		line_cstr := cstring(&buf[0])
		line := string(line_cstr)
		line = strings.trim_space(line)
		if len(line) > 0 {
			append(&results, strings.clone(line))
		}
	}

	slice.sort_by(results[:], compare_results)

	if len(results) > 10 {
		for i := 10; i < len(results); i += 1 {
			delete(results[i])
		}
		resize(&results, 10)
	}

	return results
}
