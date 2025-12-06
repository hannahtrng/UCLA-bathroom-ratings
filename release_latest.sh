# note: run gcloud auth configure-docker gcr.io once to run this

# pull latest image
docker pull "gcr.io/cheesy-bread/insert-cheesy-bread:latest"

# tag image with release
docker tag "gcr.io/cheesy-bread/insert-cheesy-bread:latest" "gcr.io/cheesy-bread/insert-cheesy-bread:release"

# push release
docker push "gcr.io/cheesy-bread/insert-cheesy-bread:release"

#restart VM to see changes

