# Bulk channel write

Hosts loading supplied material pixels need one operation per channel or rectangular range. Per-pixel preview sessions allocate whole-canvas coverage and make dense imports impractical.

Add a region write on a texture-set transaction, expose it through the C ABI and Python, Swift, and Rust. A full channel is a region spanning its extent. Writes remain undoable and subject to declared tile targets and history budget. A non-undoable initialization path and bulk read changes are outside this change.
