import sys, re, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

for pdf_path in sys.argv[1:]:
    print(f"\n=== {pdf_path} ===")
    with open(pdf_path, 'rb') as f:
        data = f.read()
        text = data.decode('latin-1', errors='ignore')
        parts = re.findall(r'\(([^)]+)\)', text)
        for p in parts:
            p = p.strip()
            if len(p) > 2 and not any(p.startswith(c) for c in ['\\', '/', '.']):
                try:
                    print(p)
                except:
                    pass
