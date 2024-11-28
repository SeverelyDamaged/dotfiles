vim.opt.clipboard = "unnamedplus"
vim.opt.number = true
vim.opt.relativenumber = true
vim.opt.expandtab = true
vim.opt.shiftwidth = 4
vim.opt.tabstop = 4
vim.opt.expandtab = true
vim.opt.undofile = true
vim.opt.mouse = "a"
vim.opt.scrolloff = 8
vim.opt.smartindent = true
vim.opt.splitright = true
vim.opt.splitbelow = true
vim.opt.signcolumn = "yes"
vim.opt.inccommand = "split"
vim.opt.fillchars = { eob = " " }
vim.opt.wrap = false
vim.opt.smartcase = true
vim.opt.ignorecase = true

-- For showing hidden characters
vim.opt.list = true
vim.opt.listchars = {
	tab = "»·",
	trail = "·",
	extends = ">",
	precedes = "<",
	nbsp = "␣",
	eol = "↲",
}
