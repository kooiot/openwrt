#git clone https://github.com/pyenv/pyenv.git ~/.pyenv 
export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init --path)"
# after then
# pyenv install 2.7.18
# cd /usr/local/bin && ln -s /home/cch/.pyenv/versions/2.7.18/bin/python2.7 ./python2.7
# pyenv global 2.7.18
