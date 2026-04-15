python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
pip install matplotlib
python3 speedup.py
deactivate
rm -rf .matplotlib
rm -rf .venv
