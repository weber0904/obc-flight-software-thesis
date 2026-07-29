from __future__ import annotations

import threading
from contextlib import contextmanager
from typing import Hashable, Iterator


class BandLockPool:
    def __init__(self) -> None:
        self._locks: dict[tuple[Hashable, ...], threading.Lock] = {}
        self._guard = threading.Lock()

    @contextmanager
    def hold(self, *key_parts: Hashable) -> Iterator[None]:
        lock = self._lock_for(tuple(key_parts))
        lock.acquire()
        try:
            yield
        finally:
            lock.release()

    def _lock_for(self, key: tuple[Hashable, ...]) -> threading.Lock:
        with self._guard:
            lock = self._locks.get(key)
            if lock is None:
                lock = threading.Lock()
                self._locks[key] = lock
            return lock
