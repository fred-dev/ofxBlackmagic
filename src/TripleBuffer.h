#pragma once

#include "ofMain.h"

// The writer fills getBack() and calls swapBack(); the reader calls swapFront()
// and uses getFront(). Neither waits for the other.
template <class T>
class TripleBuffer : public std::mutex {
private:
	T back, middle, front;
	bool newData;
public:
	TripleBuffer()
	:newData(false) {
	}
	void setup(const T& prototype) {
		back = prototype;
		middle = prototype;
		front = prototype;
		newData = false;
	}
	T& getBack() {
		return back;
	}
	T& getFront() {
		return front;
	}
	// Returns true if the previous frame was replaced before the reader took it.
	bool swapBack() {
		lock();
		swap(back, middle);
		bool skipped = newData;
		newData = true;
		unlock();
		return skipped;
	}
	bool swapFront() {
		lock();
		bool curNewData = newData;
		if(newData) {
			swap(front, middle);
			newData = false;
		}
		unlock();
		return curNewData;
	}
	// For cleaning up (only when neither side is running)
	template <class F>
	void forEach(F f) {
		lock();
		f(back);
		f(middle);
		f(front);
		newData = false;
		unlock();
	}
};
