// Copyright (c) 2026 Nordic Semiconductor ASA
// SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

sphinxToggleRunWhenDOMLoaded(() => {
  document.querySelectorAll(".toggle-custom-hints").forEach((item) => {
    const details = item.closest("details.toggle-details");
    if (!details) {
      return;
    }
    const text = details.querySelector(".toggle-details__summary-text");
    const update = () => {
      text.innerText = details.open ? item.dataset.toggleHide : item.dataset.toggleShow;
    };
    update();
    details.addEventListener("toggle", update);
  });
});
