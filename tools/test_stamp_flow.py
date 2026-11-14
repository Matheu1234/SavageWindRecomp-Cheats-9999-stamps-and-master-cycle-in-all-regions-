"""Compile and exercise the native phase tracer with a fake guest CPU."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from tools.analyze_stamp_flow import load, rank

ROOT = Path(__file__).resolve().parent.parent


class FlowTests(unittest.TestCase):
    def test_phase_transitions_and_output(self):
        with tempfile.TemporaryDirectory() as d:
            d = Path(d)
            exe = d / ("flow_test.exe" if os.name == "nt" else "flow_test")
            cc = os.environ.get("CC", "clang")
            build = subprocess.run([cc, "-std=c11", "-O1", "-Wall", "-Wextra",
                                    "-I" + str(ROOT / "runtime"),
                                    str(ROOT / "runtime/stamp_flow.c"),
                                    str(ROOT / "tests/stamp_flow_harness.c"),
                                    "-o", str(exe)], capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            env = os.environ.copy()
            env.pop("SAVAGEWIND_FNPROF", None)
            env["SAVAGEWIND_STAMP_FLOW"] = "1"
            result = subprocess.run([str(exe)], cwd=d, env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            traces = sorted(d.glob("stamp-flow-*.tsv"))
            self.assertEqual(len(traces), 3, list(d.iterdir()))
            phases = {p.stem.rsplit("-", 1)[-1]: p for p in traces}
            self.assertEqual(set(phases), {"baseline", "add", "remove"})
            baseline, samples = load(phases["baseline"])
            added, _ = load(phases["add"])
            removed, _ = load(phases["remove"])
            self.assertEqual(baseline[(0x7000dd8208, 0x7000dd81e0)], 2)
            self.assertEqual(samples[(0x7000dd8208, 0x7000dd81e0)][1], 0xabc)
            self.assertEqual(added[(0x7000da7788, 0x7000da7780)], 1)
            self.assertEqual(removed[(0x7000da7788, 0x7000da7770)], 1)
            exclusive, increased = rank(baseline, added, 10)
            self.assertIn((0x7000da7788, 0x7000da7780), exclusive)
            self.assertIn("output=", result.stderr)

    def test_instrumentation_is_opt_in(self):
        source = (ROOT / "runtime/fnprof.c").read_text()
        self.assertIn("rt_sw_stamp_flow_hit(a, lr, x0)", source)
        self.assertIn("rt_sw_stamp_flow_frame(frame)", source)
        self.assertIn("if (__builtin_expect", source)
        self.assertIn("rt_sw_stamp_flow_init();", (ROOT / "runtime/main.c").read_text())
        self.assertNotIn("rt_sw_stamp_flow_hit", (ROOT / "runtime/cpu.h").read_text())


if __name__ == "__main__":
    unittest.main()
