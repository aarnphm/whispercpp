## Note on writing tests.

[`conftest.py`](./conftest.py) will setup the test environment to ensure that the module is imported correctly.

Make sure to import `whispercpp` as `w` in your tests.

```python
import whispercpp as w
```

Refer to [`conftest.py`](./conftest.py) for more details.
