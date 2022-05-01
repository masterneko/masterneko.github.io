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

// get /blog/contents.json and iterate over the contents
// and display them in the blog-list element
let blog_list = document.getElementById("blog-list");

let blog_xhr = new XMLHttpRequest();
blog_xhr.open("GET", "/blog/contents.json");

blog_xhr.onload = () =>
{
    if(blog_xhr.status === 200)
    {
        let blog_contents = JSON.parse(blog_xhr.responseText).contents;

        for(let i = 0; i < blog_contents.length; i++)
        {
            let truncate_len = screen.width < 700 ? 50 : 140;

            let blog_card_text =
`<div class="blog-card-info">
    <h3>${blog_contents[i].path}</h3>
    <p>${blog_contents[i].content.substring(0, truncate_len)}</p>
</div>

<div><a href="blog/${blog_contents[i].url}"><button>Read</button></a></div>`;

            // create a new blog card element and assign the text to it
            let blog_card = document.createElement("div");
            blog_card.innerHTML = blog_card_text;
            blog_card.classList.add("blog-card");

            // append the blog card to the blog list
            blog_list.appendChild(blog_card);
        }
    }
    else
    {
        // append error to the blog list
        let blog_card_text =
`<h3>Error</h3>
<p>There was an problem loading the blog posts.</p>`;

        let blog_card = document.createElement("div");
        blog_card.innerHTML = blog_card_text;
        blog_list.appendChild(blog_card);
    }
}

blog_xhr.send();
