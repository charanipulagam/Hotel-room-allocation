document.addEventListener("DOMContentLoaded", function () {

    const totalRoomsElement =
        document.getElementById("totalRooms");

    const availableRoomsElement =
        document.getElementById("availableRooms");

    const occupiedRoomsElement =
        document.getElementById("occupiedRooms");

    const waitingCustomersElement =
        document.getElementById("waitingCustomers");

    const recentReservationsBody =
        document.getElementById("recentReservationsBody");


    /* =========================================
       LOAD ROOM STATISTICS
       ========================================= */

    function loadRoomStatistics() {

        /*
         * Dashboard statistics are taken from the
         * reservations saved by the frontend.
         *
         * The C backend should not overwrite these
         * values because the reservation data is
         * currently stored in localStorage.
         */

        updateFromSavedReservations();

    }


    /* =========================================
       ROOM STATISTICS FROM SAVED RESERVATIONS
       ========================================= */

    function updateFromSavedReservations() {

        const reservations =
            JSON.parse(
                localStorage.getItem(
                    "grandStayReservations"
                )
            ) || [];


        /*
         * Only confirmed reservations should
         * occupy a room.
         */

        const confirmedReservations =
            reservations.filter(function (reservation) {

                return reservation.status === "Confirmed";

            });


        /*
         * Set is used so the same room is not
         * counted more than once.
         */

        const occupiedRooms =
            new Set();


        confirmedReservations.forEach(
            function (reservation) {

                if (reservation.room !== undefined &&
                    reservation.room !== null &&
                    reservation.room !== "") {

                    occupiedRooms.add(
                        String(reservation.room)
                    );

                }

            }
        );


        const occupiedCount =
            occupiedRooms.size;

        const totalRooms = 80;

        const availableCount =
            totalRooms - occupiedCount;


        /* -------------------------
           UPDATE DASHBOARD
           ------------------------- */

        if (totalRoomsElement) {

            totalRoomsElement.textContent =
                totalRooms;

        }


        if (availableRoomsElement) {

            availableRoomsElement.textContent =
                availableCount;

        }


        if (occupiedRoomsElement) {

            occupiedRoomsElement.textContent =
                occupiedCount;

        }

    }


    /* =========================================
       LOAD WAITING LIST COUNT
       ========================================= */

    function loadWaitingCustomers() {

        const queue =
            JSON.parse(
                localStorage.getItem(
                    "grandStayWaitingQueue"
                )
            ) || [];


        if (waitingCustomersElement) {

            waitingCustomersElement.textContent =
                queue.length;

        }

    }


    /* =========================================
       LOAD RECENT RESERVATIONS
       ========================================= */

    function loadRecentReservations() {

        if (!recentReservationsBody) {

            return;

        }


        const reservations =
            JSON.parse(
                localStorage.getItem(
                    "grandStayReservations"
                )
            ) || [];


        recentReservationsBody.innerHTML = "";


        if (reservations.length === 0) {

            const row =
                document.createElement("tr");


            row.innerHTML = `
                <td colspan="5"
                    style="text-align:center; padding:20px;">
                    No reservations yet.
                </td>
            `;


            recentReservationsBody.appendChild(row);

            return;

        }


        /*
         * Show the newest reservations first.
         * Only the latest 5 are displayed.
         */

        const recentReservations =
            reservations.slice(-5).reverse();


        recentReservations.forEach(
            function (reservation, index) {

                const row =
                    document.createElement("tr");


                const reservationId =
                    reservation.id ||
                    reservation.reservationId ||
                    (
                        "GS-" +
                        String(
                            reservations.length - index
                        ).padStart(3, "0")
                    );


                const customerName =
                    reservation.name ||
                    reservation.customerName ||
                    reservation.customer ||
                    "Guest";


                const room =
                    reservation.room ||
                    reservation.roomNumber ||
                    "-";


                const roomType =
                    reservation.roomType ||
                    reservation.type ||
                    "-";


                const status =
                    reservation.status ||
                    "Confirmed";


                row.innerHTML = `

                    <td>
                        ${reservationId}
                    </td>

                    <td>
                        ${customerName}
                    </td>

                    <td>
                        Room ${room}
                    </td>

                    <td>
                        ${roomType}
                    </td>

                    <td>
                        ${status}
                    </td>

                `;


                recentReservationsBody.appendChild(row);

            }
        );

    }


    /* =========================================
       INITIAL LOAD
       ========================================= */

    loadRoomStatistics();

    loadWaitingCustomers();

    loadRecentReservations();


    /* =========================================
       AUTO REFRESH
       ========================================= */

    setInterval(function () {

        loadRoomStatistics();

        loadWaitingCustomers();

        loadRecentReservations();

    }, 5000);

});