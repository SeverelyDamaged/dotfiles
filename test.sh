for dir in *
do
    if [ -d "$dir" ] && [ "$dir" != "zsh" ] && [ "$dir" != "x11" ]; then
        if [ -d "$dir/.config/$dir" ]; then
            echo "$dir"
            cp -vr "$dir/.config/$dir" .
        fi
    fi
done