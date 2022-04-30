let welcome_text = document.getElementById("welcome-text");

// fetch https://api.github.com/user/53895123 and read the login property
// and write it to the welcome-text element
// using XMLHttpRequest
let xhr = new XMLHttpRequest();
xhr.open("GET", "https://api.github.com/user/53895123");

xhr.onload = () =>
{
    if(xhr.status === 200)
    {
        let user = JSON.parse(xhr.responseText);

        welcome_text.innerHTML = `Hi, I'm <a href="${user.html_url}">${user.login}</a>!`;
    }
}

xhr.send();

