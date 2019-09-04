# build
sudo docker build --rm --no-cache -t hello_crow:latest .

#run on Port 8080
sudo docker run -p 8080:8080 -e PORT=8080 -e MONGODB_URI="mongodb+srv://admin:1234@cpp-api-tutorial-l3zax.mongodb.net" hello_crow:latest
